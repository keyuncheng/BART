#include "ComputeWorker.hh"

ComputeWorker::ComputeWorker(
    Config &_config, uint16_t _self_conn_id,
    unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> &_pc_task_queues, unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> &_pc_reply_queues, unordered_map<uint16_t, MessageQueue<Command> *> &_cmd_dist_queues, MessageQueue<ParityComputeTask> &_parity_reloc_task_queue, MemoryPool &_memory_pool, unsigned _num_threads) : ThreadPool(_num_threads), config(_config), self_conn_id(_self_conn_id), sockets_map(_sockets_map), pc_task_queues(_pc_task_queues), pc_reply_queues(_pc_reply_queues), cmd_dist_queues(_cmd_dist_queues), parity_reloc_task_queue(_parity_reloc_task_queue), memory_pool(_memory_pool)
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

    // create buffers, sockets and handlers for each Agent
    for (auto &item : config.agent_addr_map)
    {
        uint16_t conn_id = item.first;
        if (self_conn_id != conn_id)
        {
            // init sockets for data transfer
            data_connectors_map[conn_id] = sockpp::tcp_connector();
            data_sockets_map[conn_id] = sockpp::tcp_socket();
            // init handler threads
            data_handler_threads_map[conn_id] = NULL;
        }
    }

    // create separate connections for data transfer
    unsigned int self_port = config.agent_addr_map[self_conn_id].second;
    data_acceptor = new sockpp::tcp_acceptor(self_port + 1000);

    printf("ComputeWorker: start connection at %u\n", self_port + 1000);

    thread ack_conn_thread([&]
                           { ComputeWorker::ackConnAll(); });

    // connect all nodes
    ComputeWorker::connectAll();

    // join ack connector threads
    ack_conn_thread.join();
}

ComputeWorker::~ComputeWorker()
{
    data_acceptor->close();
    delete data_acceptor;

    free(pm_buffers);
    free(re_buffers);
    free(buffer);
    destroyECTables();
}

void ComputeWorker::run()
{
    printf("ComputeWorker::run [Node %u] start to handle parity computation tasks\n", self_conn_id);

    // after socket initialization
    for (auto &item : data_handler_threads_map)
    {
        uint16_t conn_id = item.first;
        data_handler_threads_map[conn_id] = new thread(&ComputeWorker::handleDataTransfer, this, conn_id);
    }

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

            printf("ComputeWorker::run received parity computation task, post: (%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_stripe_id);
            // parity_compute_task.print();

            if (parity_compute_task.enc_method == EncodeMethod::RE_ENCODE)
            { // compute re-encoding
              //   // step 1: collect data from disk / network
              //   ParityComputeTask src_block_task;

                //     // Optimize (start): parallelize read data across the cluster
                //     unordered_map<uint16_t, vector<unsigned char *>> src_node_block_buffers_map;
                //     for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                //     {
                //         uint16_t src_node_id = parity_compute_task.src_block_nodes[data_block_id];
                //         src_node_block_buffers_map[src_node_id].push_back(re_buffers[data_block_id]);
                //     }

                //     thread *retrieve_threads = new thread[src_node_block_buffers_map.size()];

                //     uint8_t thread_id = 0;
                //     for (auto &item : src_node_block_buffers_map)
                //     {
                //         retrieve_threads[thread_id++] = thread(&ComputeWorker::retrieveMultipleDataAndReply, this, &parity_compute_task, item.first, item.second);
                //     }

                //     for (uint idx = 0; idx < src_node_block_buffers_map.size(); idx++)
                //     {
                //         retrieve_threads[idx].join();
                //     }

                //     delete[] retrieve_threads;
                //     // Optimize (end): parallelize read data across the cluster

                //     // // // Before Optimization (start) : sequentially read data
                //     // vector<string> src_block_paths(code.k_f);
                //     // for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                //     // {
                //     //     uint16_t src_node_id = parity_compute_task.src_block_nodes[data_block_id];
                //     //     retrieveDataAndReply(parity_compute_task, src_node_id, re_buffers[data_block_id]);
                //     // }
                //     // // // Before Optimization (end) : sequentially read data

                //     printf("ComputeWorker::run retrieved all required data for re-encoding, post: (%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);

                //     // step 2: encode data
                //     ec_encode_data(config.block_size, code.k_f, code.m_f, re_encode_gftbl, re_buffers, &re_buffers[code.k_f]);

                //     // step 3: write data to disk
                //     for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                //     {
                //         // write parity_id to disk
                //         string dst_block_path = parity_compute_task.parity_block_dst_paths[parity_id];
                //         printf("ComputeWorker::run dst_block_path is: %s", dst_block_path.c_str());
                //         if (BlockIO::writeBlock(dst_block_path, re_buffers[code.k_f + parity_id], config.block_size) != config.block_size)
                //         {
                //             fprintf(stderr, "ComputeWorker::run error writing block: %s\n", dst_block_path.c_str());
                //             exit(EXIT_FAILURE);
                //         }

                //         printf("ComputeWorker::run finished writing parity block %s\n", dst_block_path.c_str());
                //     }

                //     // send parity relocation task to queue
                //     parity_reloc_task_queue.Push(parity_compute_task);

                //     printf("ComputeWorker::run performed re-encoding for parity computation task, post(%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);
            }
            else if (parity_compute_task.enc_method == EncodeMethod::PARITY_MERGE)
            {
                // find nodes to retrieve data
                unordered_map<uint16_t, vector<unsigned char *>> src_node_block_buffers_map;
                for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                {
                    uint16_t src_node_id = parity_compute_task.src_block_nodes[pre_stripe_id];
                    src_node_block_buffers_map[src_node_id].push_back(pm_buffers[pre_stripe_id]);
                }

                thread *data_request_threads = new thread[src_node_block_buffers_map.size()];

                printf("ComputeWorker::run start to retrieve all required data for parity merging, post: (%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);

                // create threads to retrieve data
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

                printf("ComputeWorker::run finished retrieve all required data for parity merging, post: (%u, %u)\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);

                uint8_t parity_id = parity_compute_task.post_block_id - code.k_f;

                // compute parity merging
                ec_encode_data(config.block_size, code.lambda_i, 1, pm_encode_gftbl[parity_id], pm_buffers, &pm_buffers[code.lambda_i]);

                // write the parity block to disk
                string dst_block_path = parity_compute_task.dst_block_paths[0];
                if (BlockIO::writeBlock(dst_block_path, pm_buffers[code.lambda_i], config.block_size) != config.block_size)
                {
                    fprintf(stderr, "error writing block: %s\n", dst_block_path.c_str());
                    exit(EXIT_FAILURE);
                }

                // printf("ComputeWorker::run finished writing parity block %s\n", dst_block_path.c_str());

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

    // send stop commands to data handler threads
    for (auto &item : data_connectors_map)
    {
        uint16_t conn_id = item.first;
        auto &data_connector = item.second;
        Command cmd_disconnect;
        cmd_disconnect.buildCommand(CommandType::CMD_STOP, self_conn_id, conn_id);
        if (data_connector.write_n(cmd_disconnect.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "ComputeWorker::run error sending disconnect cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd_disconnect.type, cmd_disconnect.src_conn_id, cmd_disconnect.dst_conn_id);
            exit(EXIT_FAILURE);
        }
        data_connector.close();
    }

    // stop handlers
    for (auto &item : data_handler_threads_map)
    {
        uint16_t conn_id = item.first;
        data_handler_threads_map[conn_id]->join();
        delete data_handler_threads_map[conn_id];
    }

    printf("ComputeWorker::run [Node %u] finished handling parity computation tasks\n", self_conn_id);
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

            printf("ComputeWorker::requestDataFromAgent finished read local data at Node %u, post: (%u, %u), src_block_path: %s\n", src_node_id, parity_compute_task->post_stripe_id, parity_compute_task->post_block_id, src_block_path.c_str());
        }
        else
        { // retrieve data from other Nodes

            auto &data_connector = data_connectors_map[src_node_id];
            auto &data_socket = data_sockets_map[src_node_id];

            // send command to the node to retrieve data
            Command cmd_transfer;
            cmd_transfer.buildCommand(CommandType::CMD_TRANSFER_BLK, self_conn_id, src_node_id, parity_compute_task->post_stripe_id, parity_compute_task->post_block_id, src_node_id, self_conn_id, src_block_path, string());
            if (data_connector.write_n(cmd_transfer.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "ComputeWorker::requestDataFromAgent error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd_transfer.type, cmd_transfer.src_conn_id, cmd_transfer.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            // // receive block
            // if (BlockIO::recvBlock(data_socket, data_buffer, config.block_size) != config.block_size)
            // {
            //     fprintf(stderr, "ComputeWorker::retrieveData error recv block from ComputeWorker (%u)\n", src_node_id);
            //     exit(EXIT_FAILURE);
            // }

            printf("ComputeWorker::requestDataFromAgent finished retrieving data from Node %u, post: (%u, %u), src_block_path: %s\n", src_node_id, parity_compute_task->post_stripe_id, parity_compute_task->post_block_id, src_block_path.c_str());
        }

        cur_buffer_id++;
    }

    // printf("ComputeWorker::requestDataFromAgent finished retrieving %u blocks from Agent %u\n", cur_buffer_id, src_node_id);
}

void ComputeWorker::handleDataTransfer(uint16_t src_conn_id)
{
    printf("ComputeWorker::handleDataTransfer [Node %u] start to handle data transfer from ComputeWorker %u\n", self_conn_id, src_conn_id);

    auto &data_connector = data_connectors_map[src_conn_id];
    auto &data_skt = data_sockets_map[src_conn_id];

    unsigned char *data_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

    while (true)
    {
        Command cmd;

        ssize_t ret_val = data_skt.read_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char));
        if (ret_val == -1)
        {
            fprintf(stderr, "ComputeWorker::handleDataTransfer error reading command\n");
            exit(EXIT_FAILURE);
        }
        else if (ret_val == 0)
        {
            // currently, no cmd coming in
            printf("ComputeWorker::handleDataTransfer no command coming in from ComputeWorker %u, break\n", src_conn_id);
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

            // // send block
            // if (BlockIO::sendBlock(data_connector, data_buffer, config.block_size) != config.block_size)
            // {
            //     fprintf(stderr, "ComputeWorker::handleDataTransfer error sending block: %s to ComputeWorker %u\n", cmd.src_block_path.c_str(), cmd.src_conn_id);
            //     exit(EXIT_FAILURE);
            // }

            printf("ComputeWorker::handleDataTransfer finished transferring data to Agent %u, post: (%u, %u), src_block_path: %s\n", cmd.src_conn_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_block_path.c_str());
        }
        else if (cmd.type == CommandType::CMD_STOP)
        {
            // printf("ComputeWorker::handleDataTransfer received stop command from ComputeWorker %u, call socket.close()\n", cmd.src_conn_id);
            data_skt.close();
            break;
        }
    }

    free(data_buffer);

    printf("ComputeWorker::handleDataTransfer [Node %u] finished handling data transfer from ComputeWorker %u\n", self_conn_id, src_conn_id);
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

void ComputeWorker::connectAll()
{
    unordered_map<uint16_t, thread *> conn_threads;

    // create connect threads
    for (auto &item : data_connectors_map)
    {
        uint16_t conn_id = item.first;
        string ip;
        unsigned int port;
        if (conn_id != self_conn_id)
        {
            ip = config.agent_addr_map[conn_id].first;
            port = config.agent_addr_map[conn_id].second + 1000;
        }

        conn_threads[conn_id] = new thread(&ComputeWorker::connectOne, this, conn_id, ip, port);
    }

    for (auto &item : conn_threads)
    {
        item.second->join();
        delete item.second;
    }

    // create handle ack threads
    unordered_map<uint16_t, thread *> ack_threads;
    for (auto &item : data_connectors_map)
    {
        uint16_t conn_id = item.first;
        ack_threads[conn_id] = new thread(&ComputeWorker::handleAckOne, this, conn_id);
    }

    for (auto &item : ack_threads)
    {
        item.second->join();
        delete item.second;
    }

    printf("ComputeWorker::connectAll successfully connected to %lu nodes\n", data_connectors_map.size());
}

void ComputeWorker::connectOne(uint16_t conn_id, string ip, uint16_t port)
{
    // create connection
    sockpp::tcp_connector &connector = data_connectors_map[conn_id];
    while (!(connector = sockpp::tcp_connector(sockpp::inet_address(ip, port))))
    {
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    // printf("Successfully created connection to conn_id: %u (%s, %u)\n", conn_id, ip.c_str(), port);

    // send the connection command
    Command cmd_conn;
    cmd_conn.buildCommand(CommandType::CMD_CONN, self_conn_id, conn_id);

    if (connector.write_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "error send cmd_conn\n");
        exit(EXIT_FAILURE);
    }

    // printf("ComputeWorker::connectOne send connection command to %u \n", conn_id);
    // Utils::printUCharBuffer(cmd_conn.content, MAX_CMD_LEN);
}

void ComputeWorker::ackConnAll()
{
    uint16_t num_acked_nodes = 0;
    uint16_t num_conns = data_connectors_map.size();
    while (num_acked_nodes < num_conns)
    {
        sockpp::inet_address conn_addr;
        sockpp::tcp_socket skt = data_acceptor->accept(&conn_addr);

        if (!skt)
        {
            fprintf(stderr, "ComputeWorker::ackConnAll invalid socket: %s\n", data_acceptor->last_error_str().c_str());
            exit(EXIT_FAILURE);
        }

        // printf("ComputeWorker::ackConnAll received from %s\n", conn_addr.to_string().c_str());

        // parse the connection command
        Command cmd_conn;
        if (skt.read_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "ComputeWorker::ackConnAll error reading cmd_conn\n");
            exit(EXIT_FAILURE);
        }

        cmd_conn.parse();

        if (cmd_conn.type != CommandType::CMD_CONN || cmd_conn.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "ComputeWorker::ackConnAll invalid cmd_conn: type: %u, dst_conn_id: %u\n", cmd_conn.type, cmd_conn.dst_conn_id);
            exit(EXIT_FAILURE);
        }

        // printf("ComputeWorker::ackConnAll received connection command\n");
        // Utils::printUCharBuffer(cmd_conn.content, MAX_CMD_LEN);

        // maintain the socket
        uint16_t conn_id = cmd_conn.src_conn_id;
        data_sockets_map[conn_id] = move(skt);

        // send the ack command
        auto &connector = data_sockets_map[conn_id];

        Command cmd_ack;
        cmd_ack.buildCommand(CommandType::CMD_ACK, self_conn_id, conn_id);

        if (connector.write_n(cmd_ack.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "ComputeWorker::ackConnAll error send cmd_ack\n");
            exit(EXIT_FAILURE);
        }

        num_acked_nodes++;

        // printf("ComputeWorker::ackConnAll received connection and acked to %u; connected to (%u / %u) Nodes\n", conn_id, num_acked_nodes, num_conns);
        // Utils::printUCharBuffer(cmd_ack.content, MAX_CMD_LEN);
    }
}

void ComputeWorker::handleAckOne(uint16_t conn_id)
{
    sockpp::tcp_connector &connector = data_connectors_map[conn_id];

    // parse the ack command
    Command cmd_ack;
    if (connector.read_n(cmd_ack.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "ComputeWorker::handleAckOne error reading cmd_ack from %u\n", conn_id);
        exit(EXIT_FAILURE);
    }
    cmd_ack.parse();

    // printf("ComputeWorker::handleAckOne received command from %u \n", conn_id);
    // Utils::printUCharBuffer(cmd_ack.content, MAX_CMD_LEN);

    if (cmd_ack.type != CommandType::CMD_ACK)
    {
        fprintf(stderr, "ComputeWorker::handleAckOne invalid command type %d from connection %u\n", cmd_ack.type, conn_id);
        exit(EXIT_FAILURE);
    }

    // printf("ComputeWorker %u: received ack from ComputeWorker %u\n", self_conn_id, conn_id);

    printf("ComputeWorker::handleAckOne successfully connected to conn_id: %u\n", conn_id);
}