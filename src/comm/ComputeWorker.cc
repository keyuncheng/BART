#include "ComputeWorker.hh"

ComputeWorker::ComputeWorker(Config &_config, unsigned int _self_worker_id, uint16_t _self_conn_id, MessageQueue<Command> &_compute_task_queue, unordered_map<unsigned int, MultiWriterQueue<Command> *> &_reloc_task_queues) : ThreadPool(1), config(_config), self_worker_id(_self_worker_id), self_conn_id(_self_conn_id), compute_task_queue(_compute_task_queue), reloc_task_queues(_reloc_task_queues)
{
    ConvertibleCode &code = config.code;

    // initialize EC tables (for both re-encoding and parity merging)
    initECTables();

    // init buffers for encoding
    unsigned int buffer_size = code.n_f > (code.lambda_i + 1) ? code.n_f : (code.lambda_i + 1);
    block_buffer = (unsigned char *)malloc(buffer_size * config.block_size * sizeof(unsigned char));
    re_buffers = (unsigned char **)malloc(code.n_f * sizeof(unsigned char *));
    for (uint8_t block_id = 0; block_id < code.n_f; block_id++)
    {
        re_buffers[block_id] = &block_buffer[block_id * config.block_size];
    }

    pm_buffers = (unsigned char **)malloc((code.lambda_i + 1) * sizeof(unsigned char *));
    for (uint8_t block_id = 0; block_id < code.lambda_i + 1; block_id++)
    {
        pm_buffers[block_id] = &block_buffer[block_id * config.block_size];
    }

    // // create memory pools for parity writes
    // memory_pool = new MemoryPool(5, config.block_size);

    printf("[Node %u, Worker %u] ComputeWorker::ComputeWorker finished initialization\n", self_conn_id, self_worker_id);
}

ComputeWorker::~ComputeWorker()
{
    // delete memory_pool;
    free(pm_buffers);
    free(re_buffers);
    free(block_buffer);
    destroyECTables();
}

void ComputeWorker::run()
{
    printf("[Node %u, Worker %u] ComputeWorker::run start to handle parity computation tasks\n", self_conn_id, self_worker_id);

    ConvertibleCode &code = config.code;

    Command cmd_compute;

    unsigned int reloc_task_counter = 0;

    while (true)
    {
        if (finished() == true && compute_task_queue.IsEmpty() == true)
        {
            break;
        }

        if (compute_task_queue.Pop(cmd_compute) == true)
        {
            // terminate signal
            if (cmd_compute.type == CommandType::CMD_STOP)
            {
                // forward the terminate signal to reloc queues
                for (auto &item : reloc_task_queues)
                {
                    auto &reloc_task_queue = item.second;
                    reloc_task_queue->Push(cmd_compute);
                }

                setFinished();
                continue;
            }

            // parse the block paths
            vector<string> src_block_paths;
            vector<string> dst_block_paths;
            parseBlockPaths(cmd_compute, src_block_paths, dst_block_paths);

            printf("[Node %u, Worker %u] ComputeWorker::run received parity computation task, post: (%u, %u)\n", self_conn_id, self_worker_id, cmd_compute.post_stripe_id, cmd_compute.post_block_id);

            if (cmd_compute.enc_method == EncodeMethod::RE_ENCODE)
            { // compute re-encoding

                // step 1: retrieve data
                // find nodes to retrieve data
                unordered_map<uint16_t, vector<unsigned char *>> src_node_block_buffers_map;
                for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                {
                    uint16_t src_node_id = cmd_compute.src_block_nodes[data_block_id];
                    src_node_block_buffers_map[src_node_id].push_back(re_buffers[data_block_id]);
                }

                // printf("ComputeWorker::run start to retrieve all required data for re-encoding, post: (%u, %u)\n", cmd_compute.post_stripe_id, cmd_compute.post_block_id);

                // create threads to retrieve data
                thread *data_request_threads = new thread[src_node_block_buffers_map.size()];
                uint8_t thread_id = 0;
                for (auto &item : src_node_block_buffers_map)
                {
                    data_request_threads[thread_id++] = thread(&ComputeWorker::requestDataFromAgent, this, &cmd_compute, item.first, item.second);
                }

                // join threads
                for (uint idx = 0; idx < src_node_block_buffers_map.size(); idx++)
                {
                    data_request_threads[idx].join();
                }
                delete[] data_request_threads;

                // printf("ComputeWorker::run finished retrieving all required data for re-encoding, post: (%u, %u)\n", cmd_compute.post_stripe_id, cmd_compute.post_block_id);

                // step 2: encode data
                ec_encode_data(config.block_size, code.k_f, code.m_f, re_encode_gftbl, re_buffers, &re_buffers[code.k_f]);

                // step 3: write parity blocks to disk
                for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                {
                    // write parity_id to disk
                    string dst_block_path = dst_block_paths[parity_id];
                    if (BlockIO::writeBlock(dst_block_path, re_buffers[code.k_f + parity_id], config.block_size) != config.block_size)
                    {
                        fprintf(stderr, "ComputeWorker::run error writing block: %s\n", dst_block_path.c_str());
                        exit(EXIT_FAILURE);
                    }

                    // printf("ComputeWorker::run finished writing parity block %s\n", dst_block_path.c_str());
                }

                // step 4: relocate parity blocks

                // push the relocation task to specific compute task queue
                // unsigned int assigned_worker_id = cmd_compute.post_stripe_id % config.num_reloc_workers;
                unsigned int assigned_worker_id = reloc_task_counter % config.num_reloc_workers;

                for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                {
                    uint16_t dst_conn_id = cmd_compute.parity_reloc_nodes[parity_id];
                    string &dst_block_path = dst_block_paths[parity_id];

                    // check whether needs relocation
                    if (self_conn_id != dst_conn_id)
                    {
                        Command cmd_reloc;
                        cmd_reloc.buildCommand(CommandType::CMD_TRANSFER_RELOC_BLK, self_conn_id, dst_conn_id, cmd_compute.post_stripe_id, cmd_compute.post_block_id, self_conn_id, dst_conn_id, dst_block_path, dst_block_path);

                        // pass to corresponding relocation worker
                        reloc_task_queues[assigned_worker_id]->Push(cmd_reloc);

                        reloc_task_counter++;

                        printf("[Node %u, Worker %u] ComputeWorker::run created parity block relocation task (type: %u, Node %u -> %u), forwarded to RelocWorker %u\n", self_conn_id, self_worker_id, cmd_reloc.type, cmd_reloc.src_node_id, cmd_reloc.dst_node_id, assigned_worker_id);
                    }
                }
            }
            else if (cmd_compute.enc_method == EncodeMethod::PARITY_MERGE)
            {
                // step 1: retrieve data
                // find nodes to retrieve data
                unordered_map<uint16_t, vector<unsigned char *>> src_node_block_buffers_map;
                for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                {
                    uint16_t src_node_id = cmd_compute.src_block_nodes[pre_stripe_id];
                    src_node_block_buffers_map[src_node_id].push_back(pm_buffers[pre_stripe_id]);
                }

                // printf("ComputeWorker::run start to retrieve all required data for parity merging, post: (%u, %u)\n", cmd_compute.post_stripe_id, cmd_compute.post_block_id);

                // create threads to retrieve data
                thread *data_request_threads = new thread[src_node_block_buffers_map.size()];
                uint8_t thread_id = 0;
                for (auto &item : src_node_block_buffers_map)
                {
                    data_request_threads[thread_id++] = thread(&ComputeWorker::requestDataFromAgent, this, &cmd_compute, item.first, item.second);
                }

                // join threads
                for (uint idx = 0; idx < src_node_block_buffers_map.size(); idx++)
                {
                    data_request_threads[idx].join();
                }
                delete[] data_request_threads;

                // printf("ComputeWorker::run finished retrieve all required data for parity merging, post: (%u, %u)\n", cmd_compute.post_stripe_id, cmd_compute.post_block_id);

                // step 2: encode data
                uint8_t parity_id = cmd_compute.post_block_id - code.k_f;
                uint16_t dst_conn_id = cmd_compute.parity_reloc_nodes[0];
                ec_encode_data(config.block_size, code.lambda_i, 1, pm_encode_gftbl[parity_id], pm_buffers, &pm_buffers[code.lambda_i]);

                // // use memory pool
                // unsigned char *req_buffer;
                // if (self_conn_id == dst_conn_id)
                // {
                //     // temporarily hold the memory
                //     unsigned char *temp_buffer = pm_buffers[code.lambda_i];

                //     // request memory
                //     req_buffer = memory_pool->getBlock();

                //     // temporarily assign memory
                //     pm_buffers[code.lambda_i] = req_buffer;

                //     // encode
                //     ec_encode_data(config.block_size, code.lambda_i, 1, pm_encode_gftbl[parity_id], pm_buffers, &pm_buffers[code.lambda_i]);

                //     // return the memory
                //     pm_buffers[code.lambda_i] = temp_buffer;
                // }
                // else
                // {
                //     ec_encode_data(config.block_size, code.lambda_i, 1, pm_encode_gftbl[parity_id], pm_buffers, &pm_buffers[code.lambda_i]);
                // }

                // step 3: write data to disk
                // write the parity block to disk
                string dst_block_path = dst_block_paths[0];
                if (BlockIO::writeBlock(dst_block_path, pm_buffers[code.lambda_i], config.block_size) != config.block_size)
                {
                    fprintf(stderr, "error writing block: %s\n", dst_block_path.c_str());
                    exit(EXIT_FAILURE);
                }

                // // use memory pool
                // if (self_conn_id == dst_conn_id)
                // { // need to write to disk, and can be detached
                //     thread write_data_thread(&ComputeWorker::writeBlockToDisk, this, dst_block_path, req_buffer, config.block_size);
                //     write_data_thread.detach();
                // }
                // else
                // { // need to write to disk, and cannot be detached
                //     if (BlockIO::writeBlock(dst_block_path, pm_buffers[code.lambda_i], config.block_size) != config.block_size)
                //     {
                //         fprintf(stderr, "error writing block: %s\n", dst_block_path.c_str());
                //         exit(EXIT_FAILURE);
                //     }
                // }

                // printf("ComputeWorker::run finished writing parity block %s\n", dst_block_path.c_str());

                // step 4: relocate parity blocks
                // unsigned int assigned_worker_id = cmd_compute.post_stripe_id % config.num_reloc_workers;
                unsigned int assigned_worker_id = reloc_task_counter % config.num_reloc_workers;

                // check whether needs relocation
                if (self_conn_id != dst_conn_id)
                {
                    Command cmd_reloc;
                    cmd_reloc.buildCommand(CommandType::CMD_TRANSFER_RELOC_BLK, self_conn_id, dst_conn_id, cmd_compute.post_stripe_id, cmd_compute.post_block_id, self_conn_id, dst_conn_id, dst_block_path, dst_block_path);

                    // // pass to corresponding relocation worker
                    reloc_task_queues[assigned_worker_id]->Push(cmd_reloc);

                    reloc_task_counter++;

                    printf("[Node %u, Worker %u] ComputeWorker::run created parity block relocation task (type: %u, Node %u -> %u), forwarded to RelocWorker %u\n", self_conn_id, self_worker_id, cmd_reloc.type, cmd_reloc.src_node_id, cmd_reloc.dst_node_id, assigned_worker_id);
                }
            }

            printf("[Node %u, Worker %u] ComputeWorker::run finished parity computation task, post(%u, %u)\n", self_conn_id, self_worker_id, cmd_compute.post_stripe_id, cmd_compute.post_block_id);
        }
    }

    printf("[Node %u, Worker %u] ComputeWorker::run finished handling parity computation tasks\n", self_conn_id, self_worker_id);
}

void ComputeWorker::requestDataFromAgent(Command *cmd_compute, uint16_t src_node_id, vector<unsigned char *> data_buffers)
{
    vector<string> src_block_paths;
    vector<string> dst_block_paths;
    parseBlockPaths(*cmd_compute, src_block_paths, dst_block_paths);

    uint8_t num_blocks_to_retrieve = INVALID_BLK_ID;
    if (cmd_compute->enc_method == EncodeMethod::RE_ENCODE)
    {
        num_blocks_to_retrieve = config.code.k_f;
    }
    else if (cmd_compute->enc_method == EncodeMethod::PARITY_MERGE)
    {
        num_blocks_to_retrieve = config.code.lambda_i;
    }
    uint8_t cur_buffer_id = 0;

    for (uint8_t block_id = 0; block_id < num_blocks_to_retrieve; block_id++)
    {
        uint16_t block_src_node_id = cmd_compute->src_block_nodes[block_id];
        string src_block_path = src_block_paths[block_id];
        unsigned char *data_buffer = data_buffers[cur_buffer_id];

        // only retrieve the block from src_node_id
        if (src_node_id != block_src_node_id)
        {
            continue;
        }

        if (src_node_id == self_conn_id)
        { // read data from disk
            if (BlockIO::readBlock(src_block_path, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "ComputeWorker::requestDataFromAgent error reading local block: %s\n", src_block_path.c_str());
                exit(EXIT_FAILURE);
            }

            printf("ComputeWorker::requestDataFromAgent finished read local data at Node %u, post: (%u, %u), src_block_path: %s\n", src_node_id, cmd_compute->post_stripe_id, cmd_compute->post_block_id, src_block_path.c_str());
        }
        else
        { // retrieve data from other Nodes

            printf("ComputeWorker::requestDataFromAgent start to retrieve data from Node %u, post: (%u, %u), src_block_path: %s\n", src_node_id, cmd_compute->post_stripe_id, cmd_compute->post_block_id, src_block_path.c_str());

            // send command to the node to retrieve data
            Command cmd_transfer;
            cmd_transfer.buildCommand(CommandType::CMD_TRANSFER_BLK, self_conn_id, src_node_id, cmd_compute->post_stripe_id, cmd_compute->post_block_id, src_node_id, self_conn_id, src_block_path, string());

            // create connection to block request handler
            string block_req_ip = config.agent_addr_map[src_node_id].first;
            unsigned int block_req_port = config.agent_addr_map[src_node_id].second + config.settings.num_nodes; // DEBUG

            sockpp::tcp_connector connector;
            while (!(connector = sockpp::tcp_connector(sockpp::inet_address(block_req_ip, block_req_port))))
            {
                this_thread::sleep_for(chrono::milliseconds(1));
            }

            // send block transfer request
            if (connector.write_n(cmd_transfer.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "ComputeWorker::requestDataFromAgent error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd_transfer.type, cmd_transfer.src_conn_id, cmd_transfer.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            // receive block
            if (BlockIO::recvBlock(connector, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "ComputeWorker::retrieveData error recv block from ComputeWorker (%u)\n", src_node_id);
                exit(EXIT_FAILURE);
            }

            connector.close();

            printf("ComputeWorker::requestDataFromAgent finished retrieving data from Node %u, post: (%u, %u), src_block_path: %s\n", src_node_id, cmd_compute->post_stripe_id, cmd_compute->post_block_id, src_block_path.c_str());
        }

        cur_buffer_id++;
    }

    printf("ComputeWorker::requestDataFromAgent finished retrieving %u blocks from Agent %u\n", cur_buffer_id, src_node_id);
}

void ComputeWorker::writeBlockToDisk(string block_path, unsigned char *data_buffer, uint64_t block_size)
{
    if (BlockIO::writeBlock(block_path, data_buffer, block_size) != block_size)
    {
        fprintf(stderr, "error writing block: %s\n", block_path.c_str());
        exit(EXIT_FAILURE);
    }

    memory_pool->freeBlock(data_buffer);
}

void ComputeWorker::parseBlockPaths(Command &cmd_compute, vector<string> &src_block_paths, vector<string> &dst_block_paths)
{
    ConvertibleCode &code = config.code;

    string delimiter_char = ":";
    size_t pos = 0;
    string token;

    string raw_src_block_path = cmd_compute.src_block_path;
    string raw_dst_block_path = cmd_compute.dst_block_path;

    // process the paths
    if (cmd_compute.enc_method == EncodeMethod::RE_ENCODE)
    {
        // src block paths
        src_block_paths.resize(code.k_f);
        pos = 0;
        uint8_t data_block_id = 0;
        while ((pos = raw_src_block_path.find(delimiter_char)) != std::string::npos)
        {
            token = raw_src_block_path.substr(0, pos);
            src_block_paths[data_block_id] = token;
            raw_src_block_path.erase(0, pos + delimiter_char.length());
            data_block_id++;
        }
        src_block_paths[code.k_f - 1] = raw_src_block_path;

        // dst block paths
        dst_block_paths.resize(code.m_f);
        pos = 0;
        uint8_t parity_id = 0;

        while ((pos = raw_dst_block_path.find(delimiter_char)) != std::string::npos)
        {
            token = raw_dst_block_path.substr(0, pos);
            dst_block_paths[parity_id] = token;
            raw_dst_block_path.erase(0, pos + delimiter_char.length());
            parity_id++;
        }
        dst_block_paths[code.m_f - 1] = raw_dst_block_path;
    }
    else if (cmd_compute.enc_method == EncodeMethod::PARITY_MERGE)
    {
        // src block paths
        src_block_paths.resize(code.lambda_i);
        pos = 0;
        uint8_t pre_stripe_id = 0;
        while ((pos = raw_src_block_path.find(delimiter_char)) != std::string::npos)
        {
            token = raw_src_block_path.substr(0, pos);
            src_block_paths[pre_stripe_id] = token;
            raw_src_block_path.erase(0, pos + delimiter_char.length());
            pre_stripe_id++;
        }
        src_block_paths[code.lambda_i - 1] = raw_src_block_path;

        // dst block paths
        dst_block_paths.resize(1);
        dst_block_paths[0] = raw_dst_block_path;
    }
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
