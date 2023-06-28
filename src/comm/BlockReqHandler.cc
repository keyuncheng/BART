#include "BlockReqHandler.hh"

BlockReqHandler::BlockReqHandler(Config &_config, uint16_t _self_conn_id, unsigned int _num_workers) : ThreadPool(1), config(_config), self_conn_id(_self_conn_id), num_workers(_num_workers)
{
    // maximum allowed connections
    max_conn = 100; // DEBUG
    cur_conn = 0;

    // queues for worker threads
    for (unsigned int worker_id = 0; worker_id < num_workers; worker_id++)
    {
        req_handler_queues[worker_id] = new MessageQueue<BlockReq>(MAX_MSG_QUEUE_LEN);
    }

    // add acceptor
    unsigned int self_port = config.agent_addr_map[self_conn_id].second + config.settings.num_nodes; // DEBUG

    acceptor = new sockpp::tcp_acceptor(self_port);
}

BlockReqHandler::~BlockReqHandler()
{
    acceptor->close();
    delete acceptor;

    for (unsigned int worker_id = 0; worker_id < num_workers; worker_id++)
    {
        delete req_handler_queues[worker_id];
    }
}

void BlockReqHandler::run()
{
    printf("[Node %u] BlockReqHandler::run start to handle block requests\n", self_conn_id);

    // create block request handlers
    for (unsigned int worker_id = 0; worker_id < num_workers; worker_id++)
    {
        req_handler_threads_map[worker_id] = new thread(&BlockReqHandler::handleBlockRequest, this, worker_id);
    }

    // stop counter
    uint16_t stop_counter = 0;

    // block request counter
    uint32_t block_request_counter = 0;

    while (finished() == false)
    {
        // // check lock
        // unique_lock<mutex> lck(max_conn_mutex);
        // while (cur_conn > max_conn)
        // {
        //     max_conn_cv.wait(lck, [&]
        //                      { return cur_conn <= max_conn; });
        // }

        // accept socket
        sockpp::inet_address conn_addr;
        sockpp::tcp_socket *skt = new sockpp::tcp_socket();

        *skt = acceptor->accept(&conn_addr);
        if (!skt)
        {
            fprintf(stderr, "BlockReqHandler::run invalid socket: %s\n", acceptor->last_error_str().c_str());
            exit(EXIT_FAILURE);
        }

        // command
        Command cmd;

        ssize_t ret_val = skt->read_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char));
        if (ret_val == -1 || ret_val == 0)
        {
            fprintf(stderr, "BlockReqHandler::run error reading command\n");
            exit(EXIT_FAILURE);
        }

        // parse the command
        cmd.parse();
        // cmd.print();

        // validate command
        if (cmd.dst_conn_id != self_conn_id)
        {
            // fprintf(stderr, "BlockReqHandler::run error: invalid command content\n");
            fprintf(stderr, "BlockReqHandler::run error: cmd.type: %u, cmd.src_conn_id: %u, cmd.dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
            exit(EXIT_FAILURE);
        }

        // stop request
        if (cmd.type == CommandType::CMD_STOP)
        {
            stop_counter++;
            if (stop_counter == config.settings.num_nodes - 1)
            { // set finished if received stop command from other Agent nodes
                setFinished();
            }
            continue;
        }

        BlockReq req;
        req.cmd = cmd;
        req.skt = skt;

        // distribute the socket to worker
        unsigned int assigned_worker_id = block_request_counter % num_workers;

        // obtain block request socket
        printf("[Node %u] BlockReqHandler::run obtained block request, assign to Worker %u, post: (%u, %u)\n", self_conn_id, assigned_worker_id, cmd.post_stripe_id, cmd.post_block_id);

        req_handler_queues[assigned_worker_id]->Push(req);

        // increment counter
        block_request_counter++;

        // // increment number of connections
        // cur_conn++;
    }

    // distribute stop requests
    Command cmd_stop;
    cmd_stop.buildCommand(CommandType::CMD_STOP, self_conn_id, self_conn_id);

    BlockReq req;
    req.cmd = cmd_stop;
    req.skt = NULL;
    for (unsigned int worker_id = 0; worker_id < num_workers; worker_id++)
    {
        req_handler_queues[worker_id]->Push(req);
    }

    // join block request handlers
    for (unsigned int worker_id = 0; worker_id < num_workers; worker_id++)
    {
        req_handler_threads_map[worker_id]->join();
        delete req_handler_threads_map[worker_id];
    }

    printf("[Node %u] BlockReqHandler::run finished handling block requests\n", self_conn_id);
}

void BlockReqHandler::handleBlockRequest(unsigned int self_worker_id)
{
    printf("[Node %u, Worker: %u] BlockReqHandler::handleBlockRequest start to handle block requests\n", self_conn_id, self_worker_id);

    auto &req_handler_queue = req_handler_queues[self_worker_id];

    bool is_finished = false;
    unsigned char *data_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

    BlockReq req;
    while (true)
    {
        // accept block request
        if (is_finished == true && req_handler_queue->IsEmpty())
        {
            break;
        }

        if (req_handler_queue->Pop(req) == true)
        {
            Command &cmd = req.cmd;
            sockpp::tcp_socket *skt = req.skt;

            // terminate signal
            if (cmd.type == CommandType::CMD_STOP && skt == NULL)
            {
                is_finished = true;
                continue;
            }

            // obtain block request socket
            printf("[Node %u] BlockReqHandler::handleBlockRequest obtained block request, type: %u, post: (%u, %u)\n", self_conn_id, cmd.type, cmd.post_stripe_id, cmd.post_block_id);

            if (cmd.type == CommandType::CMD_TRANSFER_BLK)
            {
                // read block
                if (BlockIO::readBlock(cmd.src_block_path, data_buffer, config.block_size) != config.block_size)
                {
                    fprintf(stderr, "BlockReqHandler::handleBlockRequest error reading block: %s\n", cmd.src_block_path.c_str());
                    exit(EXIT_FAILURE);
                }

                // send block
                if (BlockIO::sendBlock(*skt, data_buffer, config.block_size) != config.block_size)
                {
                    fprintf(stderr, "BlockReqHandler::handleBlockRequest error sending block: %s to BlockReqHandler %u\n", cmd.src_block_path.c_str(), cmd.src_conn_id);
                    exit(EXIT_FAILURE);
                }

                // obtain block request socket
                printf("[Node %u, Worker: %u] BlockReqHandler::handleBlockRequest handled block transfer request, post: (%u, %u), src_block_path: %s\n", self_conn_id, self_worker_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_block_path.c_str());
            }
            else if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
            {
                // retrieve block from the same socket, and write to disk

                // recv block
                if (BlockIO::recvBlock(*skt, data_buffer, config.block_size) != config.block_size)
                {
                    fprintf(stderr, "BlockReqHandler::handleBlockRequest error receiving block: %s to RelocWorker %u\n", cmd.src_block_path.c_str(), cmd.src_conn_id);
                    exit(EXIT_FAILURE);
                }

                // write block
                if (BlockIO::writeBlock(cmd.dst_block_path, data_buffer, config.block_size) != config.block_size)
                {
                    fprintf(stderr, "BlockReqHandler::handleBlockRequest error writing block: %s\n", cmd.dst_block_path.c_str());
                    exit(EXIT_FAILURE);
                }

                // obtain block request socket
                printf("[Node %u, Worker: %u] BlockReqHandler::handleBlockRequest handled block relocation request, post: (%u, %u), dst_block_path: %s\n", self_conn_id, self_worker_id, cmd.post_stripe_id, cmd.post_block_id, cmd.dst_block_path.c_str());
            }

            // close socket
            skt->close();
            delete skt;

            // // reduce connections
            // unique_lock<mutex> lck(max_conn_mutex);
            // cur_conn--;
            // lck.unlock();
            // max_conn_cv.notify_one();
        }
    }

    free(data_buffer);

    printf("[Node %u, Worker: %u] BlockReqHandler::handleBlockRequest finished handling block requests\n", self_conn_id, self_worker_id);
}

void BlockReqHandler::stopHandling()
{
    // inform other nodes to stop handling block requests
    for (auto &item : config.agent_addr_map)
    {
        uint16_t conn_id = item.first;
        if (conn_id != self_conn_id)
        {
            auto &addr = item.second;

            // create connection to block request handler
            string block_req_ip = addr.first;
            unsigned int block_req_port = addr.second + config.settings.num_nodes; // DEBUG

            sockpp::tcp_connector connector;
            while (!(connector = sockpp::tcp_connector(sockpp::inet_address(block_req_ip, block_req_port))))
            {
                this_thread::sleep_for(chrono::milliseconds(1));
            }

            // send stop request to block handlers of all other nodes
            Command cmd_stop;
            cmd_stop.buildCommand(CommandType::CMD_STOP, self_conn_id, conn_id);

            // send block transfer request
            if (connector.write_n(cmd_stop.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "ComputeWorker::requestDataFromAgent error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd_stop.type, cmd_stop.src_conn_id, cmd_stop.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            connector.close();
        }
    }

    printf("[Node %u] BlockReqHandler::stopHandling trigger stop handling request\n", self_conn_id);
}