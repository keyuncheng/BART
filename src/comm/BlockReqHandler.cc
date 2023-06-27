#include "BlockReqHandler.hh"

BlockReqHandler::BlockReqHandler(Config &_config, uint16_t _self_conn_id, unsigned int _num_workers) : ThreadPool(1), config(_config), self_conn_id(_self_conn_id), num_workers(_num_workers)
{
    // maximum allowed connections
    max_conn = 100; // DEBUG
    cur_conn = 0;

    // queues for worker threads
    for (unsigned int worker_id = 0; worker_id < num_workers; worker_id++)
    {
        req_handler_queues[worker_id] = new MessageQueue<sockpp::tcp_socket *>(MAX_MSG_QUEUE_LEN);
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

    uint32_t block_request_counter = 0;

    while (finished() == false)
    {
        // check lock
        unique_lock<mutex> lck(max_conn_mutex);
        while (cur_conn > max_conn)
        {
            max_conn_cv.wait(lck, [&]
                             { return cur_conn <= max_conn; });
        }

        // accept socket
        sockpp::inet_address conn_addr;
        sockpp::tcp_socket *skt = new sockpp::tcp_socket();
        *skt = acceptor->accept(&conn_addr);

        if (!skt)
        {
            fprintf(stderr, "BlockReqHandler::run invalid socket: %s\n", acceptor->last_error_str().c_str());
            exit(EXIT_FAILURE);
        }

        // distribute the socket to worker
        unsigned int assigned_worker_id = block_request_counter % num_workers;

        req_handler_queues[assigned_worker_id]->Push(skt);

        // increment number of connections
        cur_conn++;
    }

    // distribute stop requests
    for (unsigned int worker_id = 0; worker_id < num_workers; worker_id++)
    {
        sockpp::tcp_socket *term_skt = new sockpp::tcp_socket();
        req_handler_queues[worker_id]->Push(term_skt);
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

    sockpp::tcp_socket *skt;
    while (true)
    {
        // accept block request
        if (is_finished == true && req_handler_queue->IsEmpty())
        {
            break;
        }

        if (req_handler_queue->Pop(skt) == true)
        {
            // terminate signal
            if (skt->is_open() == false)
            {
                is_finished = true;
                continue;
            }
        }

        // read command
        Command cmd;

        ssize_t ret_val = skt->read_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char));
        if (ret_val == -1 || ret_val == 0)
        {
            fprintf(stderr, "BlockReqHandler::handleBlockRequest error reading command\n");
            exit(EXIT_FAILURE);
        }

        // parse the command
        cmd.parse();
        // cmd.print();

        // validate command
        if (cmd.dst_conn_id != self_conn_id)
        {
            // fprintf(stderr, "BlockReqHandler::handleBlockRequest error: invalid command content\n");
            fprintf(stderr, "BlockReqHandler::handleBlockRequest error: cmd.type: %u, cmd.src_conn_id: %u, cmd.dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
            exit(EXIT_FAILURE);
        }

        // obtain block request socket
        printf("[Node %u, Worker: %u] BlockReqHandler::handleBlockRequest obtained block request\n", self_conn_id, self_worker_id);

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
            printf("[Node %u, Worker: %u] BlockReqHandler::handleBlockRequest handled block transfer request\n", self_conn_id, self_worker_id);
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
                fprintf(stderr, "error writing block: %s\n", cmd.dst_block_path.c_str());
                exit(EXIT_FAILURE);
            }

            // obtain block request socket
            printf("[Node %u, Worker: %u] BlockReqHandler::handleBlockRequest handled block relocation request\n", self_conn_id, self_worker_id);
        }

        // close socket
        skt->close();

        // reduce connections
        unique_lock<mutex> lck(max_conn_mutex);
        cur_conn--;
        lck.unlock();
        max_conn_cv.notify_one();
    }

    free(data_buffer);

    printf("[Node %u, Worker: %u] BlockReqHandler::handleBlockRequest finished handling block requests\n", self_conn_id, self_worker_id);
}