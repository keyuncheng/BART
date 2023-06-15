#include "ComputeWorker.hh"

ComputeWorker::ComputeWorker(
    Config &_config, uint16_t _self_conn_id,
    unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> &_pc_task_queues, unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> &_pc_reply_queues, MessageQueue<ParityComputeTask> &_parity_reloc_task_queue, MemoryPool &_memory_pool, unsigned _num_threads) : ThreadPool(_num_threads), config(_config), self_conn_id(_self_conn_id), sockets_map(_sockets_map), pc_task_queues(_pc_task_queues), pc_reply_queues(_pc_reply_queues), parity_reloc_task_queue(_parity_reloc_task_queue), memory_pool(_memory_pool)
{
    ConvertibleCode &code = config.code;

    // initialize EC tables (for both re-encoding and parity merging)
    initECTables();

    // init buffers for encoding
    unsigned int buffer_size = code.n_f > (code.lambda_i + 1) ? code.n_f : (code.lambda_i + 1);
    buffer = (unsigned char *)malloc(buffer_size * config.block_size * sizeof(unsigned char));
    re_buffers = (unsigned char **)malloc(code.n_f * sizeof(unsigned char *));
    for (uint8_t block_id = 0; block_id < code.n_f; block_id++)
    {
        re_buffers[block_id] = &buffer[block_id * config.block_size];
    }

    pm_buffers = (unsigned char **)malloc((code.lambda_i + 1) * sizeof(unsigned char *));
    for (uint8_t block_id = 0; block_id < code.lambda_i + 1; block_id++)
    {
        pm_buffers[block_id] = &buffer[block_id * config.block_size];
    }
}

ComputeWorker::~ComputeWorker()
{
    free(pm_buffers);
    free(re_buffers);
    free(buffer);
    destroyECTables();
}

void ComputeWorker::run()
{
    printf("ComputeWorker::run start to handle parity computation tasks\n");

    ConvertibleCode &code = config.code;

    // controller message equeue (for retrieving parity compute task)
    MessageQueue<ParityComputeTask> &pc_controller_task_queue = *pc_task_queues[CTRL_NODE_ID];

    // allocate parity block buffers for parity merging
    ParityComputeTask parity_compute_task;

    while (true)
    {
        if (finished() == true && pc_controller_task_queue.IsEmpty() == true)
        {
            break;
        }

        if (pc_controller_task_queue.Pop(parity_compute_task) == true)
        {
            // terminate signal
            if (parity_compute_task.all_tasks_finished == true)
            {
                // forward the terminate signal to parity reloc queue
                parity_reloc_task_queue.Push(parity_compute_task);
                setFinished();
                continue;
            }

            printf("ComputeWorker::run received parity computation task\n");
            parity_compute_task.print();

            if (parity_compute_task.enc_method == EncodeMethod::RE_ENCODE)
            { // compute re-encoding
                // step 1: collect data from disk / network
                ParityComputeTask src_block_task;

                // retrieve tasks
                vector<string> src_block_paths(code.k_f);
                for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                {
                    uint16_t src_node_id = parity_compute_task.src_block_nodes[data_block_id];
                    retrieveDataAndReply(parity_compute_task, src_node_id, re_buffers[data_block_id]);

                    printf("\n\n\npath %s\n", src_block_paths[data_block_id].c_str());
                }

                // retrieve data

                printf("ComputeWorker::run retrieved all required data for re-encoding, post: (%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);

                // step 2: encode data
                ec_encode_data(config.block_size, code.k_f, code.m_f, re_encode_gftbl, re_buffers, &re_buffers[code.k_f]);

                // step 3: write data to disk
                for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                {
                    // write parity_id to disk
                    string dst_block_path = parity_compute_task.parity_block_dst_paths[parity_id];
                    if (BlockIO::writeBlock(dst_block_path, re_buffers[code.k_f + parity_id], config.block_size) != config.block_size)
                    {
                        fprintf(stderr, "ComputeWorker::run error writing block: %s\n", dst_block_path.c_str());
                        exit(EXIT_FAILURE);
                    }

                    printf("ComputeWorker::run finished writing parity block %s\n", dst_block_path.c_str());
                }

                // send parity relocation task to queue
                parity_reloc_task_queue.Push(parity_compute_task);

                printf("ComputeWorker::run performed re-encoding for parity computation task, post(%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);
            }
            else if (parity_compute_task.enc_method == EncodeMethod::PARITY_MERGE)
            {
                // step 1: collect data from disk / network
                ParityComputeTask src_block_task;

                // retrieve task
                for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                {
                    uint16_t src_node_id = parity_compute_task.src_block_nodes[pre_stripe_id];

                    retrieveDataAndReply(parity_compute_task, src_node_id, pm_buffers[pre_stripe_id]);
                }

                uint8_t parity_id = parity_compute_task.post_block_id - code.k_f;

                // compute parity merging
                ec_encode_data(config.block_size, code.lambda_i, 1, pm_encode_gftbl[parity_id], pm_buffers, &pm_buffers[code.lambda_i]);

                string dst_block_path = parity_compute_task.parity_block_dst_paths[0];

                // write the parity block to disk
                if (BlockIO::writeBlock(dst_block_path, pm_buffers[code.lambda_i], config.block_size) != config.block_size)
                {
                    fprintf(stderr, "error writing block: %s\n", dst_block_path.c_str());
                    exit(EXIT_FAILURE);
                }

                printf("ComputeWorker::run finished writing parity block %s\n", dst_block_path.c_str());

                // check whether needs relocation
                if (self_conn_id != parity_compute_task.parity_reloc_nodes[0])
                {
                    // send parity relocation task to queue
                    parity_reloc_task_queue.Push(parity_compute_task);
                }

                printf("ComputeWorker::run performed parity merging for parity computation task, post(%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);
            }
        }
    }

    printf("ComputeWorker::run finished handling parity computation tasks\n");
}

void ComputeWorker::initECTables()
{
    ConvertibleCode &code = config.code;

    // init re-encoding table
    re_matrix = (unsigned char *)malloc(code.n_f * code.k_f * sizeof(unsigned char));
    re_encode_gftbl = (unsigned char *)malloc(32 * code.k_f * code.m_f * sizeof(unsigned char));
    gf_gen_rs_matrix(re_matrix, code.n_f, code.k_f); // Vandermonde matrix
    ec_init_tables(code.k_f, code.m_f, &re_matrix[code.k_f * code.k_f], re_encode_gftbl);

    // init parity merging table
    pm_matrix = (unsigned char **)malloc(code.m_f * sizeof(unsigned char *));
    pm_encode_gftbl = (unsigned char **)malloc(code.m_f * sizeof(unsigned char *));
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        pm_matrix[parity_id] = (unsigned char *)malloc(code.lambda_i * sizeof(unsigned char));

        pm_encode_gftbl[parity_id] = (unsigned char *)malloc(32 * code.lambda_i * sizeof(unsigned char));

        pm_matrix[parity_id][0] = 1; // the first element is 1
        // the following elements are parity_id ^ (k_i * pre_stripe_id)
        // for example: (3,2) -> (9,2)
        // parity generation matrix:
        // 1^0 1^(3*1) 1^(3*2)
        // 2^0 2^(3*1) 2^(3*2)
        for (uint8_t pre_stripe_id = 1; pre_stripe_id < code.lambda_i; pre_stripe_id++)
        {
            pm_matrix[parity_id][pre_stripe_id] = gfPow(parity_id + 1, code.k_i * pre_stripe_id);
        }

        ec_init_tables(code.lambda_i, 1, pm_matrix[parity_id], pm_encode_gftbl[parity_id]);
    }
}

void ComputeWorker::destroyECTables()
{
    ConvertibleCode &code = config.code;

    // free merging tables
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        free(pm_encode_gftbl[parity_id]);
        free(pm_matrix[parity_id]);
    }
    free(pm_encode_gftbl);
    free(pm_matrix);

    // free re-encoding table
    free(re_encode_gftbl);
    free(re_matrix);
}

unsigned char ComputeWorker::gfPow(unsigned char val, unsigned int times)
{
    unsigned char ret_val = 1;

    for (unsigned int i = 0; i < times; i++)
    {
        ret_val = gf_mul(ret_val, val);
    }

    return ret_val;
}

void ComputeWorker::retrieveDataAndReply(ParityComputeTask &parity_compute_task, uint16_t src_node_id, unsigned char *buffer)
{
    ParityComputeTask src_block_task;

    MessageQueue<ParityComputeTask> &pc_src_task_queue = *pc_task_queues[src_node_id];

    printf("ComputeWorker::retrieveTask start to retrieve task from pc_task_queue %u, post: (%u, %u)\n", src_node_id, src_block_task.post_stripe_id, src_block_task.post_block_id);

    while (true)
    {
        if (pc_src_task_queue.Pop(src_block_task) == true)
        {
            // printf("ComputeWorker::run received src block task");
            // src_block_task.print();

            if (src_block_task.post_stripe_id == parity_compute_task.post_stripe_id)
            {
                break;
            }
            else
            {
                fprintf(stderr, "ComputeWorker::retrieveTask error: invalid data block retrieval content\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    printf("ComputeWorker::retrieveTask retrieved task from pc_task_queue %u, post: (%u, %u)\n", src_node_id, src_block_task.post_stripe_id, src_block_task.post_block_id);

    if (self_conn_id == src_node_id)
    { // local read task
        // read block
        string src_block_path = src_block_task.parity_block_src_path;
        if (BlockIO::readBlock(src_block_path, buffer, config.block_size) != config.block_size)
        {
            fprintf(stderr, "ComputeWorker::retrieveData error reading local block: %s\n", src_block_path.c_str());
            exit(EXIT_FAILURE);
        }
    }
    else
    { // transfer task
        printf("transfer\n\n\n");
        // recv block
        auto &skt = sockets_map[src_node_id];
        if (BlockIO::recvBlock(skt, buffer, config.block_size) != config.block_size)
        {
            fprintf(stderr, "CmdHandler::retrieveData error recv block from Node (%u)\n", src_node_id);
            exit(EXIT_FAILURE);
        }
    }

    printf("ComputeWorker::retrieveData retrieved data from Node %u\n", src_node_id);

    MessageQueue<ParityComputeTask> &pc_reply_queue = *pc_reply_queues[src_node_id];

    pc_reply_queue.Push(parity_compute_task);

    printf("ComputeWorker::sendReplyTask reply task to queue %u, post: (%u, %u)\n", src_node_id, parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);
}