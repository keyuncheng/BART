#include "RelocWorker.hh"

RelocWorker::RelocWorker(Config &_config, unsigned int _self_worker_id, uint16_t _self_conn_id, MultiWriterQueue<Command> &_reloc_task_queue) : ThreadPool(0), config(_config), self_worker_id(_self_worker_id), self_conn_id(_self_conn_id), reloc_task_queue(_reloc_task_queue)
{
    // create buffers, sockets and handlers for each Agent
    unsigned int data_cmd_port_offset = 1000 + (self_worker_id + 1) * (config.settings.num_nodes * 2); // DEBUG: hard code port
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
}

RelocWorker::~RelocWorker()
{
    data_content_acceptor->close();
    delete data_content_acceptor;

    data_cmd_acceptor->close();
    delete data_cmd_acceptor;
}

void RelocWorker::run()
{
    printf("[Node %u, Worker %u] RelocWorker::run start to handle relocation tasks\n", self_conn_id, self_worker_id);

    // create data transfer handlers
    for (auto &item : data_handler_threads_map)
    {
        uint16_t conn_id = item.first;
        data_handler_threads_map[conn_id] = new thread(&RelocWorker::handleDataTransfer, this, conn_id);
    }

    unsigned char *data_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

    unsigned int num_term_signals = 0;

    while (true)
    {
        if (finished() == true && reloc_task_queue.IsEmpty() == true)
        {
            break;
        }

        Command cmd_reloc;

        if (reloc_task_queue.Pop(cmd_reloc) == true)
        {
            // terminate signal
            if (cmd_reloc.type == CommandType::CMD_STOP)
            {
                num_term_signals++;
            }

            // term signals (one from CmdHandler; another from RelocWorker)
            if (num_term_signals == 2)
            {
                setFinished();
                continue;
            }

            if (cmd_reloc.type != CommandType::CMD_TRANSFER_RELOC_BLK)
            {
                fprintf(stderr, "RelocWorker::run error invalid command type: %u\n", cmd_reloc.type);
                exit(EXIT_FAILURE);
            }

            printf("[Node %u, Worker %u] RelocWorker::run received relocation task, post: (%u, %u)\n", self_conn_id, self_worker_id, cmd_reloc.post_stripe_id, cmd_reloc.post_stripe_id);

            // data command connector (send command)
            auto &data_cmd_connector = data_cmd_connectors_map[cmd_reloc.dst_conn_id];

            // send command to the node
            Command cmd_transfer;
            cmd_transfer.buildCommand(CommandType::CMD_TRANSFER_BLK, self_conn_id, cmd_reloc.dst_conn_id, cmd_reloc.post_stripe_id, cmd_reloc.post_block_id, cmd_reloc.src_node_id, cmd_reloc.dst_node_id, cmd_reloc.dst_block_path, cmd_reloc.dst_block_path);
            if (data_cmd_connector.write_n(cmd_transfer.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "RelocWorker::run error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd_transfer.type, cmd_transfer.src_conn_id, cmd_transfer.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            // read block
            if (BlockIO::readBlock(cmd_reloc.dst_block_path, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "RelocWorker::handleDataTransfer error reading block: %s\n", cmd_reloc.dst_block_path.c_str());
                exit(EXIT_FAILURE);
            }

            auto &data_content_connector = data_content_connectors_map[cmd_reloc.dst_conn_id];

            // send block
            if (BlockIO::sendBlock(data_content_connector, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "RelocWorker::handleDataTransfer error sending block: %s to RelocWorker %u\n", cmd_reloc.dst_block_path.c_str(), cmd_reloc.src_conn_id);
                exit(EXIT_FAILURE);
            }
        }
    }

    free(data_buffer);

    printf("[Node %u, Worker %u] RelocWorker::run finished handling relocation tasks\n", self_conn_id, self_worker_id);
}

void RelocWorker::handleDataTransfer(uint16_t src_conn_id)
{
    printf("[Node %u, Worker %u] RelocWorker::handleDataTransfer start to handle data transfer from RelocWorker %u\n", self_conn_id, self_worker_id, src_conn_id);

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
            fprintf(stderr, "RelocWorker::handleDataTransfer error reading command\n");
            exit(EXIT_FAILURE);
        }
        else if (ret_val == 0)
        {
            // // currently, no cmd coming in
            // printf("RelocWorker::handleDataTransfer no command coming in from RelocWorker %u, break\n", src_conn_id);
            break;
        }

        // parse the command
        cmd.parse();
        // cmd.print();

        // validate command
        if (cmd.src_conn_id != src_conn_id || cmd.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "RelocWorker::handleDataTransfer error: invalid command content\n");
            fprintf(stderr, "RelocWorker::handleDataTransfer error: src conn: %u, cmd.type: %u, cmd.src_conn_id: %u, cmd.dst_conn_id: %u\n", src_conn_id, cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
            exit(EXIT_FAILURE);
        }

        if (cmd.type == CommandType::CMD_TRANSFER_BLK)
        {
            // printf("RelocWorker::handleDataTransfer received data request from Node %u, post: (%u, %u), src_block_path: %s\n", cmd.src_conn_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_block_path.c_str());

            // data content socket (receive block)
            auto &data_content_socket = data_content_sockets_map[cmd.src_node_id];

            // recv block
            if (BlockIO::recvBlock(data_content_socket, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "RelocWorker::handleDataTransfer error receiving block: %s to RelocWorker %u\n", cmd.src_block_path.c_str(), cmd.src_conn_id);
                exit(EXIT_FAILURE);
            }

            // read block
            if (BlockIO::writeBlock(cmd.dst_block_path, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "RelocWorker::handleDataTransfer error writing block: %s\n", cmd.dst_block_path.c_str());
                exit(EXIT_FAILURE);
            }

            // printf("RelocWorker::handleDataTransfer finished transferring data to Agent %u, post: (%u, %u), src_block_path: %s\n", cmd.src_conn_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_block_path.c_str());
        }
        else if (cmd.type == CommandType::CMD_STOP)
        {
            // printf("RelocWorker::handleDataTransfer received stop command from RelocWorker %u, call socket.close()\n", cmd.src_conn_id);

            // close sockets
            data_cmd_skt.close();
            data_content_connector.close();

            break;
        }
    }

    free(data_buffer);

    printf("[Node %u, Worker %u] RelocWorker::handleDataTransfer finished handling data transfer from RelocWorker %u\n", self_conn_id, self_worker_id, src_conn_id);
}