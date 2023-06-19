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

    // init buffers for data transfer
    data_transfer_buffers = (unsigned char **)malloc((config.agent_addr_map.size()) * sizeof(unsigned char *));
    for (uint8_t agent_id = 0; agent_id < config.agent_addr_map.size(); agent_id++)
    {
        if (agent_id != self_conn_id)
        {
            data_transfer_buffers[agent_id] = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));
        }
    }

    // init sockets for listening block data
    sockpp::socket_initializer::initialize();

    for (auto &item : config.agent_addr_map)
    {
        uint16_t conn_id = item.first;
        if (self_conn_id != conn_id)
        {
            blk_connectors_map[conn_id] = sockpp::tcp_connector();
            blk_sockets_map[conn_id] = sockpp::tcp_socket();
        }
    }
    unsigned int self_port = 0;
    self_port = std::get<2>(config.agent_addr_map[self_conn_id]);
    blk_acceptor = new sockpp::tcp_acceptor(self_port);

    printf("ComputeWorker %u: start connection\n", self_conn_id);

    thread ack_conn_thread([&]
                           { ComputeWorker::ackConnAll(); });

    // connect all nodes
    ComputeWorker::connectAll();

    // join ack connector threads
    ack_conn_thread.join();

    // after socket initialization
    for (auto &item : blk_sockets_map)
    {
        uint16_t conn_id = item.first;
        if (self_conn_id != self_conn_id) 
        {
            blk_handler_threads_map[conn_id] = new thread(&ComputeWorker::handleBlkTransfer, this, conn_id, data_transfer_buffers[conn_id]);
        }
    }
}

ComputeWorker::~ComputeWorker()
{
    blk_acceptor->close();
    delete blk_acceptor;
    free(data_transfer_buffers);
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

                // Optimize (start): parallelize read data across the cluster
                unordered_map<uint16_t, vector<unsigned char *>> src_node_block_buffers_map;
                for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                {
                    uint16_t src_node_id = parity_compute_task.src_block_nodes[data_block_id];
                    src_node_block_buffers_map[src_node_id].push_back(re_buffers[data_block_id]);
                }

                thread *retrieve_threads = new thread[src_node_block_buffers_map.size()];

                uint8_t thread_id = 0;
                for (auto &item : src_node_block_buffers_map)
                {
                    retrieve_threads[thread_id++] = thread(&ComputeWorker::retrieveMultipleDataAndReply, this, &parity_compute_task, item.first, item.second);
                }

                for (uint idx = 0; idx < src_node_block_buffers_map.size(); idx++)
                {
                    retrieve_threads[idx].join();
                }

                delete[] retrieve_threads;
                // Optimize (end): parallelize read data across the cluster

                // // // Before Optimization (start) : sequentially read data
                // vector<string> src_block_paths(code.k_f);
                // for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                // {
                //     uint16_t src_node_id = parity_compute_task.src_block_nodes[data_block_id];
                //     retrieveDataAndReply(parity_compute_task, src_node_id, re_buffers[data_block_id]);
                // }
                // // // Before Optimization (end) : sequentially read data

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

                // Optimize (start): parallelize read data across the cluster
                unordered_map<uint16_t, vector<unsigned char *>> src_node_block_buffers_map;
                for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                {
                    uint16_t src_node_id = parity_compute_task.src_block_nodes[pre_stripe_id];
                    src_node_block_buffers_map[src_node_id].push_back(pm_buffers[pre_stripe_id]);
                }

                // hcpuyang: add send requests to source nodes for data retrival
                thread *notfiy_threads = new thread[src_node_block_buffers_map.size()];

                uint8_t thread_id = 0;
                for (auto &item : src_node_block_buffers_map) 
                {
                    notfiy_threads[thread_id++] = thread(&ComputeWorker::notifyMultipleAgent, this, &parity_compute_task, item.first, item.second);
                }
                for (uint idx = 0; idx < src_node_block_buffers_map.size(); idx++)
                {
                    notfiy_threads[idx].join();
                }
                delete[] notfiy_threads;


                thread *retrieve_threads = new thread[src_node_block_buffers_map.size()];

                thread_id = 0;
                for (auto &item : src_node_block_buffers_map)
                {
                    retrieve_threads[thread_id++] = thread(&ComputeWorker::retrieveMultipleDataAndReply, this, &parity_compute_task, item.first, item.second);
                }

                for (uint idx = 0; idx < src_node_block_buffers_map.size(); idx++)
                {
                    retrieve_threads[idx].join();
                }
                delete[] retrieve_threads;

                // Optimize (end): parallelize read data across the cluster

                // // Before Optimization (start) : sequentially read data
                // // retrieve task
                // for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                // {
                //     uint16_t src_node_id = parity_compute_task.src_block_nodes[pre_stripe_id];

                //     retrieveDataAndReply(parity_compute_task, src_node_id, pm_buffers[pre_stripe_id]);
                // }
                // // Before Optimization (end) : sequentially read data

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

    printf("ComputeWorker::retrieveTask start to retrieve task from pc_task_queue %u, post: (%u, %u)\n", src_node_id, src_block_task.post_stripe_id, src_block_task.post_block_id);

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
        // recv block
        auto &blk_skt = blk_sockets_map[src_node_id];
        if (BlockIO::recvBlock(blk_skt, buffer, config.block_size) != config.block_size)
        {
            fprintf(stderr, "ComputeWorker::retrieveData error recv block from ComputeWorker (%u)\n", src_node_id);
            exit(EXIT_FAILURE);
        }
    }

    printf("ComputeWorker::retrieveData retrieved data from ComputeWorker %u\n", src_node_id);

    MessageQueue<ParityComputeTask> &pc_reply_queue = *pc_reply_queues[src_node_id];

    pc_reply_queue.Push(parity_compute_task);

    printf("ComputeWorker::sendReplyTask reply task to queue %u, post: (%u, %u)\n", src_node_id, parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);
}

void ComputeWorker::notifyAgent(ParityComputeTask &parity_compute_task, uint16_t src_node_id, unsigned char *buffer)
{
    ParityComputeTask src_block_task;

    // MessageQueue<ParityComputeTask> &pc_src_task_queue = *pc_task_queues[src_node_id];

    printf("ComputeWorker::notifyAgent start to notify agent %u", src_node_id);

    if (self_conn_id != src_node_id)
    {
        // notify other nodes to send
        auto &blk_connector = blk_connectors_map[src_node_id];
        Command cmd_forward;
        cmd_forward.buildCommand(CommandType::CMD_TRANSFER_BLK, self_conn_id, src_node_id, parity_compute_task.parity_block_src_path);

        if (blk_connector.write_n(cmd_forward.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "ComputeWorker::notifyAgent error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd_forward.type, cmd_forward.src_conn_id, cmd_forward.dst_conn_id);
            exit(EXIT_FAILURE);
        }
    }
}

void ComputeWorker::retrieveMultipleDataAndReply(ParityComputeTask *parity_compute_task, uint16_t src_node_id, vector<unsigned char *> buffers)
{
    for (auto buffer : buffers)
    {
        retrieveDataAndReply(*parity_compute_task, src_node_id, buffer);
    }
}

void ComputeWorker::notifyMultipleAgent(ParityComputeTask *parity_compute_task, uint16_t src_node_id, vector<unsigned char *> buffers) 
{
    for (auto buffer : buffers)
    {
        notifyAgent(*parity_compute_task, src_node_id, buffer);
    }
}

void ComputeWorker::handleBlkTransfer(uint16_t src_conn_id, unsigned char *buffer)
{
    printf("ComputeWorker::handleBlkTransfer [ComputeWorker %u] start to listen for block transfer command from ComputeWorker %u\n", self_conn_id, src_conn_id);

    auto &blk_skt = blk_sockets_map[src_conn_id];

    while (true)
    {
        Command cmd;

        ssize_t ret_val = blk_skt.read_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char));
        if (ret_val == -1)
        {
            fprintf(stderr, "ComputeWorker::handleBlkTransfer error reading command\n");
            exit(EXIT_FAILURE);
        }
        else if (ret_val == 0)
        {
            // currently, no cmd coming in
            printf("ComputeWorker::handleBlkTransfer no command coming in, break\n");
            break;
        }

        // parse the command
        cmd.parse();
        cmd.print();

        // validate command
        if (cmd.src_conn_id != src_conn_id || cmd.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "ComputeWorker::handleBlkTransfer error: invalid command content\n");
            exit(EXIT_FAILURE);
        }

        if (cmd.type == CommandType::CMD_TRANSFER_BLK)
        {
            printf("ComputeWorker::handleBlkTransfer recevied tranfer command from Agent %u, transfer block %s\n", cmd.src_conn_id, cmd.dst_block_path.c_str());
            // read block
            if (BlockIO::readBlock(cmd.dst_block_path, buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "ComputeWorker::handleBlkTransfer error reading block: %s\n", cmd.dst_block_path.c_str());
                exit(EXIT_FAILURE);
            }
            
            // send block
            auto &blk_connector = blk_connectors_map[cmd.src_conn_id];
            if (BlockIO::sendBlock(blk_connector, buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "ComputeWorker::handleBlkTransfer error sending block: %s to ComputeWorker %u\n", cmd.dst_block_path.c_str(), cmd.src_conn_id);
                exit(EXIT_FAILURE);
            }

            printf("ComputeWorker::handleBlkTransfer send block to ComputeWorker %u, block_path: %s\n", cmd.src_conn_id, cmd.dst_block_path.c_str());
        }
        else if (cmd.type == CommandType::CMD_STOP)
        {
            printf("ComputeWorker::handleBlkTransfer received stop command from ComputeWorker %u, call socket.close()\n", cmd.src_conn_id);
            blk_skt.close();
            break;
        }
    }
}

void ComputeWorker::connectAll() 
{
    unordered_map<uint16_t, thread *> conn_threads;

    // create connect threads
    for (auto &item : blk_connectors_map)
    {
        uint16_t conn_id = item.first;
        string ip;
        unsigned int port;
        if (conn_id != self_conn_id)
        {
            ip = std::get<0>(config.agent_addr_map[conn_id]);
            port = std::get<2>(config.agent_addr_map[conn_id]);
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
    for (auto &item : blk_connectors_map)
    {
        uint16_t conn_id = item.first;
        ack_threads[conn_id] = new thread(&ComputeWorker::handleAckOne, this, conn_id);
    }

    for (auto &item : ack_threads)
    {
        item.second->join();
        delete item.second;
    }

    printf("ComputeWorker %u: successfully connected to %lu nodes\n", self_conn_id, blk_connectors_map.size());
}

void ComputeWorker::connectOne(uint16_t conn_id, string ip, uint16_t port)
{
    // create connection
    sockpp::tcp_connector &connector = blk_connectors_map[conn_id];
    while (!(connector = sockpp::tcp_connector(sockpp::inet_address(ip, port))))
    {
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    // printf("Successfully created connection to conn_id: %u\n", conn_id);

    // send the connection command
    Command cmd_conn;
    cmd_conn.buildCommand(CommandType::CMD_CONN, self_conn_id, conn_id);

    // printf("ComputeWorker %u connected to ComputeWorker %u: \n", self_conn_id, conn_id);
    // Utils::printUCharBuffer(cmd_conn.content, MAX_CMD_LEN);

    if (connector.write_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "error send cmd_conn\n");
        exit(EXIT_FAILURE);
    }
}


void ComputeWorker::ackConnAll()
{
    uint16_t num_acked_nodes = 0;
    uint16_t num_conns = blk_connectors_map.size();
    while (num_acked_nodes < num_conns)
    {
        sockpp::inet_address conn_addr;
        sockpp::tcp_socket skt = blk_acceptor->accept(&conn_addr);

        if (!skt)
        {
            fprintf(stderr, "invalid socket: %s\n", blk_acceptor->last_error_str().c_str());
            exit(EXIT_FAILURE);
        }

        // parse the connection command
        Command cmd_conn;
        if (skt.read_n(cmd_conn.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "error reading cmd_conn\n");
            exit(EXIT_FAILURE);
        }

        // printf("Received at ComputeWorker %u: \n", self_conn_id);
        // Utils::printUCharBuffer(cmd_conn.content, MAX_CMD_LEN);

        cmd_conn.parse();

        if (cmd_conn.type != CommandType::CMD_CONN || cmd_conn.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "invalid cmd_conn: type: %u, dst_conn_id: %u\n", cmd_conn.type, cmd_conn.dst_conn_id);
            exit(EXIT_FAILURE);
        }

        // maintain the socket
        uint16_t conn_id = cmd_conn.src_conn_id;
        blk_sockets_map[conn_id] = move(skt);

        // send the ack command
        auto &connector = blk_sockets_map[conn_id];

        Command cmd_ack;
        cmd_ack.buildCommand(CommandType::CMD_ACK, self_conn_id, conn_id);

        // printf("Reply ack at ComputeWorker %u -> %u \n", self_conn_id, conn_id);
        // Utils::printUCharBuffer(cmd_ack.content, MAX_CMD_LEN);

        if (connector.write_n(cmd_ack.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
        {
            fprintf(stderr, "error send cmd_ack\n");
            exit(EXIT_FAILURE);
        }

        num_acked_nodes++;

        // printf("ComputeWorker %u: received connection and acked to ComputeWorker %u; connected to (%u / %u) Nodes\n", self_conn_id, conn_id, num_acked_nodes, num_conns);
    }
}

void ComputeWorker::handleAckOne(uint16_t conn_id)
{
    sockpp::tcp_connector &connector = blk_connectors_map[conn_id];

    // parse the ack command
    Command cmd_ack;
    if (connector.read_n(cmd_ack.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
    {
        fprintf(stderr, "error reading cmd_ack from %u\n", conn_id);
        exit(EXIT_FAILURE);
    }
    cmd_ack.parse();

    // printf("Handle ack at ComputeWorker %u -> %u \n", self_conn_id, conn_id);
    // Utils::printUCharBuffer(cmd_ack.content, MAX_CMD_LEN);

    if (cmd_ack.type != CommandType::CMD_ACK)
    {
        fprintf(stderr, "invalid command type %d from connection %u\n", cmd_ack.type, conn_id);
        exit(EXIT_FAILURE);
    }

    // printf("ComputeWorker %u: received ack from ComputeWorker %u\n", self_conn_id, conn_id);

    printf("ComputeWorker:: successfully connected to conn_id: %u\n", conn_id);
}