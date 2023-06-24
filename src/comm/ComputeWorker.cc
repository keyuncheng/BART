#include "ComputeWorker.hh"

ComputeWorker::ComputeWorker(Config &_config, unsigned int _self_worker_id, uint16_t _self_conn_id, MessageQueue<ParityComputeTask> &_compute_task_queue, unordered_map<unsigned int, MultiWriterQueue<Command> *> &_reloc_task_queues) : ThreadPool(1), config(_config), self_worker_id(_self_worker_id), self_conn_id(_self_conn_id), compute_task_queue(_compute_task_queue), reloc_task_queues(_reloc_task_queues)
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

    // create buffers, sockets and handlers for each Agent
    unsigned int data_cmd_port_offset = (self_worker_id + 1) * (config.settings.num_nodes * 4);
    unsigned int data_content_port_offset = data_cmd_port_offset + config.settings.num_nodes;
    for (auto &item : config.agent_addr_map)
    {
        uint16_t conn_id = item.first;
        auto &agent_addr = item.second;
        if (self_conn_id != conn_id)
        {
            // init handler threads
            data_handler_threads_map[conn_id] = NULL;

            // init sockets for data commands distribution
            data_cmd_addrs_map[conn_id] = pair<string, unsigned int>(agent_addr.first, agent_addr.second + data_cmd_port_offset);
            data_cmd_connectors_map[conn_id] = sockpp::tcp_connector();
            data_cmd_sockets_map[conn_id] = sockpp::tcp_socket();

            // init sockets for data content distribution
            data_content_addrs_map[conn_id] = pair<string, unsigned int>(agent_addr.first, agent_addr.second + data_content_port_offset);
            data_content_connectors_map[conn_id] = sockpp::tcp_connector();
            data_content_sockets_map[conn_id] = sockpp::tcp_socket();
        }
    }

    // create separate connections for data transfer
    unsigned int self_port = config.agent_addr_map[self_conn_id].second;

    /*********data transfer commands**********/
    // create data command acceptor
    data_cmd_acceptor = new sockpp::tcp_acceptor(self_port + data_cmd_port_offset);

    thread data_cmd_ack_conn_thread(Node::ackConnAllSockets, self_conn_id, &data_cmd_sockets_map, data_cmd_acceptor);
    // connect all nodes
    Node::connectAllSockets(self_conn_id, &data_cmd_connectors_map, &data_cmd_addrs_map);

    // join ack connector threads
    data_cmd_ack_conn_thread.join();
    /****************************************/

    /*********data content commands**********/
    // create data content acceptor
    data_content_acceptor = new sockpp::tcp_acceptor(self_port + data_content_port_offset);

    thread data_content_ack_conn_thread(Node::ackConnAllSockets, self_conn_id, &data_content_sockets_map, data_content_acceptor);
    // connect all nodes
    Node::connectAllSockets(self_conn_id, &data_content_connectors_map, &data_content_addrs_map);

    // join ack connector threads
    data_content_ack_conn_thread.join();
    /****************************************/

    printf("[Node %u, Worker %u] ComputeWorker::ComputeWorker finished initialization\n", self_conn_id, self_worker_id);
}

ComputeWorker::~ComputeWorker()
{
    data_content_acceptor->close();
    delete data_content_acceptor;

    data_cmd_acceptor->close();
    delete data_cmd_acceptor;

    free(pm_buffers);
    free(re_buffers);
    free(block_buffer);
    destroyECTables();
}

void ComputeWorker::run()
{
    printf("[Node %u, Worker %u] ComputeWorker::run start to handle parity computation tasks\n", self_conn_id, self_worker_id);

    // create data transfer handlers
    for (auto &item : data_handler_threads_map)
    {
        uint16_t conn_id = item.first;
        data_handler_threads_map[conn_id] = new thread(&ComputeWorker::handleDataTransfer, this, conn_id);
    }

    ConvertibleCode &code = config.code;

    // allocate parity block buffers for parity merging
    ParityComputeTask parity_compute_task;

    while (true)
    {
        if (finished() == true && compute_task_queue.IsEmpty() == true)
        {
            break;
        }

        if (compute_task_queue.Pop(parity_compute_task) == true)
        {
            // terminate signal
            if (parity_compute_task.all_tasks_finished == true)
            {
                // forward the terminate signal to reloc queues
                if (self_worker_id == 0)
                { // only use the first worker to send the signals
                    for (auto &item : reloc_task_queues)
                    {
                        Command cmd_stop;
                        cmd_stop.buildCommand(CommandType::CMD_STOP, INVALID_NODE_ID, INVALID_NODE_ID);

                        auto &reloc_task_queue = item.second;
                        reloc_task_queue->Push(cmd_stop);
                    }
                }
                setFinished();
                continue;
            }

            printf("[Node %u, Worker %u] ComputeWorker::run received parity computation task, post: (%u, %u)\n", self_conn_id, self_worker_id, parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);
            // parity_compute_task.print();

            if (parity_compute_task.enc_method == EncodeMethod::RE_ENCODE)
            { // compute re-encoding

                // step 1: retrieve data
                // find nodes to retrieve data
                unordered_map<uint16_t, vector<unsigned char *>> src_node_block_buffers_map;
                for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                {
                    uint16_t src_node_id = parity_compute_task.src_block_nodes[data_block_id];
                    src_node_block_buffers_map[src_node_id].push_back(re_buffers[data_block_id]);
                }

                // printf("ComputeWorker::run start to retrieve all required data for re-encoding, post: (%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);

                // create threads to retrieve data
                thread *data_request_threads = new thread[src_node_block_buffers_map.size()];
                uint8_t thread_id = 0;
                for (auto &item : src_node_block_buffers_map)
                {
                    data_request_threads[thread_id++] = thread(&ComputeWorker::requestDataFromAgent, this, &parity_compute_task, item.first, item.second);
                }

                // join threads
                for (uint idx = 0; idx < src_node_block_buffers_map.size(); idx++)
                {
                    data_request_threads[idx].join();
                }
                delete[] data_request_threads;

                // printf("ComputeWorker::run finished retrieving all required data for re-encoding, post: (%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);

                // step 2: encode data
                ec_encode_data(config.block_size, code.k_f, code.m_f, re_encode_gftbl, re_buffers, &re_buffers[code.k_f]);

                // step 3: write parity blocks to disk
                for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                {
                    // write parity_id to disk
                    string dst_block_path = parity_compute_task.dst_block_paths[parity_id];
                    if (BlockIO::writeBlock(dst_block_path, re_buffers[code.k_f + parity_id], config.block_size) != config.block_size)
                    {
                        fprintf(stderr, "ComputeWorker::run error writing block: %s\n", dst_block_path.c_str());
                        exit(EXIT_FAILURE);
                    }

                    // printf("ComputeWorker::run finished writing parity block %s\n", dst_block_path.c_str());
                }

                // step 4: relocate parity blocks

                // push the relocation task to specific compute task queue
                unsigned int assigned_worker_id = parity_compute_task.post_stripe_id % config.num_reloc_workers;

                for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                {
                    uint16_t dst_conn_id = parity_compute_task.parity_reloc_nodes[parity_id];
                    string &dst_block_path = parity_compute_task.dst_block_paths[parity_id];

                    // check whether needs relocation
                    if (self_conn_id != dst_conn_id)
                    {
                        Command cmd_reloc;
                        cmd_reloc.buildCommand(CommandType::CMD_TRANSFER_RELOC_BLK, self_conn_id, dst_conn_id, parity_compute_task.post_stripe_id, parity_compute_task.post_block_id, self_conn_id, dst_conn_id, dst_block_path, dst_block_path);

                        // pass to corresponding relocation worker
                        reloc_task_queues[assigned_worker_id]->Push(cmd_reloc);

                        printf("[Node %u, Worker %u] ComputeWorker::run created parity block relocation task (type: %u, Node %u -> %u), forwarded to RelocWorker %u\n", self_conn_id, self_worker_id, cmd_reloc.type, cmd_reloc.src_node_id, cmd_reloc.dst_node_id, assigned_worker_id);
                    }
                }
            }
            else if (parity_compute_task.enc_method == EncodeMethod::PARITY_MERGE)
            {
                // step 1: retrieve data
                // find nodes to retrieve data
                unordered_map<uint16_t, vector<unsigned char *>> src_node_block_buffers_map;
                for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                {
                    uint16_t src_node_id = parity_compute_task.src_block_nodes[pre_stripe_id];
                    src_node_block_buffers_map[src_node_id].push_back(pm_buffers[pre_stripe_id]);
                }

                // printf("ComputeWorker::run start to retrieve all required data for parity merging, post: (%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);

                // create threads to retrieve data
                thread *data_request_threads = new thread[src_node_block_buffers_map.size()];
                uint8_t thread_id = 0;
                for (auto &item : src_node_block_buffers_map)
                {
                    data_request_threads[thread_id++] = thread(&ComputeWorker::requestDataFromAgent, this, &parity_compute_task, item.first, item.second);
                }

                // join threads
                for (uint idx = 0; idx < src_node_block_buffers_map.size(); idx++)
                {
                    data_request_threads[idx].join();
                }
                delete[] data_request_threads;

                // printf("ComputeWorker::run finished retrieve all required data for parity merging, post: (%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);

                // step 2: encode data
                uint8_t parity_id = parity_compute_task.post_block_id - code.k_f;
                ec_encode_data(config.block_size, code.lambda_i, 1, pm_encode_gftbl[parity_id], pm_buffers, &pm_buffers[code.lambda_i]);

                // step 3: write data to disk
                // write the parity block to disk
                string dst_block_path = parity_compute_task.dst_block_paths[0];
                if (BlockIO::writeBlock(dst_block_path, pm_buffers[code.lambda_i], config.block_size) != config.block_size)
                {
                    fprintf(stderr, "error writing block: %s\n", dst_block_path.c_str());
                    exit(EXIT_FAILURE);
                }

                // printf("ComputeWorker::run finished writing parity block %s\n", dst_block_path.c_str());

                // step 4: relocate parity blocks
                unsigned int assigned_worker_id = parity_compute_task.post_stripe_id % config.num_reloc_workers;

                // check whether needs relocation
                uint16_t dst_conn_id = parity_compute_task.parity_reloc_nodes[0];
                if (self_conn_id != dst_conn_id)
                {
                    Command cmd_reloc;
                    cmd_reloc.buildCommand(CommandType::CMD_TRANSFER_RELOC_BLK, self_conn_id, dst_conn_id, parity_compute_task.post_stripe_id, parity_compute_task.post_block_id, self_conn_id, dst_conn_id, dst_block_path, dst_block_path);

                    // // pass to corresponding relocation worker
                    // reloc_task_queues[assigned_worker_id]->Push(cmd_reloc);

                    printf("[Node %u, Worker %u] ComputeWorker::run created parity block relocation task (type: %u, Node %u -> %u), forwarded to RelocWorker %u\n", self_conn_id, self_worker_id, cmd_reloc.type, cmd_reloc.src_node_id, cmd_reloc.dst_node_id, assigned_worker_id);
                }
            }

            printf("[Node %u, Worker %u] ComputeWorker::run finished parity computation task, post(%u, %u)\n", self_conn_id, self_worker_id, parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);
        }
    }

    // send stop commands to data handler threads
    for (auto &item : data_cmd_connectors_map)
    {
        uint16_t conn_id = item.first;
        auto &data_cmd_connector = item.second;
        Command cmd_disconnect;
        cmd_disconnect.buildCommand(CommandType::CMD_STOP, self_conn_id, conn_id);
        if (data_cmd_connector.write_n(cmd_disconnect.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "ComputeWorker::run error sending disconnect cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd_disconnect.type, cmd_disconnect.src_conn_id, cmd_disconnect.dst_conn_id);
            exit(EXIT_FAILURE);
        }

        // close data command connector
        data_cmd_connector.close();

        // close data content socket (don't receive data)
        auto &data_content_socket = data_content_sockets_map[conn_id];
        data_content_socket.close();
    }

    // stop handlers
    for (auto &item : data_handler_threads_map)
    {
        uint16_t conn_id = item.first;
        data_handler_threads_map[conn_id]->join();
        delete data_handler_threads_map[conn_id];
    }

    printf("[Node %u, Worker %u] ComputeWorker::run finished handling parity computation tasks\n", self_conn_id, self_worker_id);
}

void ComputeWorker::requestDataFromAgent(ParityComputeTask *parity_compute_task, uint16_t src_node_id, vector<unsigned char *> data_buffers)
{
    uint8_t num_blocks_to_retrieve = INVALID_BLK_ID;
    if (parity_compute_task->enc_method == EncodeMethod::RE_ENCODE)
    {
        num_blocks_to_retrieve = config.code.k_f;
    }
    else if (parity_compute_task->enc_method == EncodeMethod::PARITY_MERGE)
    {
        num_blocks_to_retrieve = config.code.m_f;
    }
    uint8_t cur_buffer_id = 0;

    for (uint8_t block_id = 0; block_id < num_blocks_to_retrieve; block_id++)
    {
        uint16_t block_src_node_id = parity_compute_task->src_block_nodes[block_id];
        string src_block_path = parity_compute_task->src_block_paths[block_id];
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

            // printf("ComputeWorker::requestDataFromAgent finished read local data at Node %u, post: (%u, %u), src_block_path: %s\n", src_node_id, parity_compute_task->post_stripe_id, parity_compute_task->post_block_id, src_block_path.c_str());
        }
        else
        { // retrieve data from other Nodes

            // data command connector (send command)
            auto &data_cmd_connector = data_cmd_connectors_map[src_node_id];

            // data content socket (receive block)
            auto &data_content_socket = data_content_sockets_map[src_node_id];

            // send command to the node to retrieve data
            Command cmd_transfer;
            cmd_transfer.buildCommand(CommandType::CMD_TRANSFER_BLK, self_conn_id, src_node_id, parity_compute_task->post_stripe_id, parity_compute_task->post_block_id, src_node_id, self_conn_id, src_block_path, string());
            if (data_cmd_connector.write_n(cmd_transfer.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "ComputeWorker::requestDataFromAgent error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd_transfer.type, cmd_transfer.src_conn_id, cmd_transfer.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            // receive block
            if (BlockIO::recvBlock(data_content_socket, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "ComputeWorker::retrieveData error recv block from ComputeWorker (%u)\n", src_node_id);
                exit(EXIT_FAILURE);
            }

            // printf("ComputeWorker::requestDataFromAgent finished retrieving data from Node %u, post: (%u, %u), src_block_path: %s\n", src_node_id, parity_compute_task->post_stripe_id, parity_compute_task->post_block_id, src_block_path.c_str());
        }

        cur_buffer_id++;
    }

    // printf("ComputeWorker::requestDataFromAgent finished retrieving %u blocks from Agent %u\n", cur_buffer_id, src_node_id);
}

void ComputeWorker::handleDataTransfer(uint16_t src_conn_id)
{
    printf("[Node %u, Worker %u] ComputeWorker::handleDataTransfer start to handle data transfer from ComputeWorker %u\n", self_conn_id, self_worker_id, src_conn_id);

    // data command socket (receive command)
    auto &data_cmd_skt = data_cmd_sockets_map[src_conn_id];

    // data content connector (send block)
    auto &data_content_connector = data_content_connectors_map[src_conn_id];

    unsigned char *data_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

    while (true)
    {
        Command cmd;

        ssize_t ret_val = data_cmd_skt.read_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char));
        if (ret_val == -1)
        {
            fprintf(stderr, "ComputeWorker::handleDataTransfer error reading command\n");
            exit(EXIT_FAILURE);
        }
        else if (ret_val == 0)
        {
            // // currently, no cmd coming in
            // printf("ComputeWorker::handleDataTransfer no command coming in from ComputeWorker %u, break\n", src_conn_id);
            break;
        }

        // parse the command
        cmd.parse();
        // cmd.print();

        // validate command
        if (cmd.src_conn_id != src_conn_id || cmd.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "ComputeWorker::handleDataTransfer error: invalid command content\n");
            fprintf(stderr, "ComputeWorker::handleDataTransfer error: src conn: %u, cmd.type: %u, cmd.src_conn_id: %u, cmd.dst_conn_id: %u\n", src_conn_id, cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
            exit(EXIT_FAILURE);
        }

        if (cmd.type == CommandType::CMD_TRANSFER_BLK)
        {
            // printf("ComputeWorker::handleDataTransfer received data request from Node %u, post: (%u, %u), src_block_path: %s\n", cmd.src_conn_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_block_path.c_str());

            // read block
            if (BlockIO::readBlock(cmd.src_block_path, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "ComputeWorker::handleDataTransfer error reading block: %s\n", cmd.src_block_path.c_str());
                exit(EXIT_FAILURE);
            }

            // send block
            if (BlockIO::sendBlock(data_content_connector, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "ComputeWorker::handleDataTransfer error sending block: %s to ComputeWorker %u\n", cmd.src_block_path.c_str(), cmd.src_conn_id);
                exit(EXIT_FAILURE);
            }

            // printf("ComputeWorker::handleDataTransfer finished transferring data to Agent %u, post: (%u, %u), src_block_path: %s\n", cmd.src_conn_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_block_path.c_str());
        }
        else if (cmd.type == CommandType::CMD_STOP)
        {
            // printf("ComputeWorker::handleDataTransfer received stop command from ComputeWorker %u, call socket.close()\n", cmd.src_conn_id);

            // close sockets
            data_cmd_skt.close();
            data_content_connector.close();

            break;
        }
    }

    free(data_buffer);

    printf("[Node %u, Worker %u] ComputeWorker::handleDataTransfer finished handling data transfer from ComputeWorker %u\n", self_conn_id, self_worker_id, src_conn_id);
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
