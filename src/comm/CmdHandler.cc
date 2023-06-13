#include "CmdHandler.hh"

CmdHandler::CmdHandler(Config &_config, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, unordered_map<uint16_t, MessageQueue<Command> *> *_cmd_dist_queues, MultiWriterQueue<ParityComputeTask> *_parity_compute_queue, MemoryPool *_memory_pool, ComputeWorker *_compute_worker, unsigned int _num_threads) : ThreadPool(_num_threads), config(_config), sockets_map(_sockets_map), cmd_dist_queues(_cmd_dist_queues), parity_compute_queue(_parity_compute_queue), memory_pool(_memory_pool), compute_worker(_compute_worker)
{
    for (auto &item : sockets_map)
    {
        uint16_t conn_id = item.first;
        handler_threads_map[conn_id] = NULL;
    }

    is_ready = false;
}

CmdHandler::~CmdHandler()
{
}

void CmdHandler::run()
{
    // init ready mutex
    unique_lock<mutex> lck(ready_mtx);
    is_ready = true;
    lck.unlock();
    ready_cv.notify_all();

    if (sockets_map.find(CTRL_NODE_ID) != sockets_map.end())
    {
        // create cmd handler thread from Controller
        handler_threads_map[CTRL_NODE_ID] = new thread(&CmdHandler::handleCmdFromController, this);
        handler_threads_map[CTRL_NODE_ID]->join();
    }

    // create cmd handler thread for Agent
    for (auto &item : sockets_map)
    {
        uint16_t conn_id = item.first;

        if (conn_id != CTRL_NODE_ID)
        { // create handler thread for Agents
            handler_threads_map[conn_id] = new thread(&CmdHandler::handleCmdFromAgent, this, conn_id);
        }
    }

    // join cmd handler thread  for Agent
    for (auto &item : sockets_map)
    {
        uint16_t conn_id = item.first;
        if (conn_id != CTRL_NODE_ID)
        {
            handler_threads_map[conn_id]->join();
        }
    }
}

void CmdHandler::handleCmdFromController()
{
    printf("CmdHandler::handleCmdFromController start to handle commands from Controller\n");

    // obtain the starter lock
    unique_lock<mutex> lck(ready_mtx);
    while (is_ready == false)
    {
        ready_cv.wait(lck, [&]
                      { return is_ready.load(); });
    }
    lck.unlock();

    uint16_t src_conn_id = CTRL_NODE_ID;
    auto &skt = sockets_map[src_conn_id];

    while (true)
    {
        Command cmd;
        // retrieve command
        ssize_t ret_val = skt.read_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char));
        if (ret_val == -1)
        {
            fprintf(stderr, "CmdHandler::handleCmdFromController error reading command\n");
            exit(EXIT_FAILURE);
        }
        else if (ret_val == 0)
        {
            // currently, no cmd comming in
            printf("CmdHandler::handleCmdFromController no command coming in, break\n");
            break;
        }

        // parse the command
        cmd.parse();
        cmd.print();

        // validate command
        if (cmd.src_conn_id != src_conn_id)
        {
            fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid command content\n");
            exit(EXIT_FAILURE);
        }

        // obtain self_conn_id from command
        uint16_t self_conn_id = cmd.dst_conn_id;

        // handle commands
        if (cmd.type == CommandType::CMD_LOCAL_COMPUTE_BLK)
        { // read block command
            // validate command
            if (cmd.src_node_id != cmd.dst_node_id)
            {
                fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid local compute command content\n");
                exit(EXIT_FAILURE);
            }

            // request a block from memory pool
            // unsigned char *block_buffer = memory_pool->getBlock();

            unsigned char *block_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

            // read block
            if (BlockIO::readBlock(cmd.src_block_path, block_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "error reading block: %s\n", cmd.src_block_path.c_str());
                exit(EXIT_FAILURE);
            }

            // pass to compute queue
            ParityComputeTask parity_compute_task(&config.code, cmd.post_stripe_id, cmd.post_block_id, block_buffer, cmd.dst_block_path);
            parity_compute_queue->Push(parity_compute_task);

            printf("CmdHandler::handleCmdFromController read local block for parity compute: %s\n", cmd.src_block_path.c_str());
        }
        else if (cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
        { // block transfer command
            // validate command
            if (cmd.src_node_id == cmd.dst_node_id)
            {
                fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid transfer command content\n");
                exit(EXIT_FAILURE);
            }

            // special handling for parity block relocation task
            // need to wait for parity compute queue to finish
            if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK && cmd.post_block_id >= config.code.k_f)
            {
                // DEBUG: hack
                continue;

                while (true)
                { // make sure the parity block has been generated and stored from compute worker
                    if (compute_worker->isTaskOngoing(EncodeMethod::RE_ENCODE, cmd.post_stripe_id, cmd.post_block_id) == false && compute_worker->isTaskOngoing(EncodeMethod::PARITY_MERGE, cmd.post_stripe_id, cmd.post_block_id) == false)
                    {
                        printf("\n\n\nfound path: %s -> %s\n\n\n\n", cmd.src_block_path.c_str(), cmd.dst_block_path.c_str());
                        break;
                    }
                    else
                    {
                        this_thread::sleep_for(chrono::milliseconds(10));
                        continue;
                    }
                }
            }

            // add lock (to dst node)
            uint16_t dst_conn_id = cmd.dst_node_id;

            // forward the command to another node
            Command cmd_forward;

            cmd_forward.buildCommand(cmd.type, self_conn_id, dst_conn_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_node_id, cmd.dst_node_id, cmd.src_block_path, cmd.dst_block_path);

            printf("received block transfer task (type: %u, Node %u -> %u), forward block\n", cmd_forward.type, cmd_forward.src_node_id, cmd_forward.dst_node_id);

            // add to cmd_dist_queue[dst_node_id]
            (*cmd_dist_queues)[dst_conn_id]->Push(cmd_forward);
        }
        else if (cmd.type == CommandType::CMD_DELETE_BLK)
        { // handle delete block request
            BlockIO::deleteBlock(cmd.src_block_path);
        }
        else if (cmd.type == CMD_STOP)
        {
            printf("CmdHandler::handleCmdFromController received stop command from Controller, call socket.close()\n");

            for (auto &item : sockets_map)
            {
                uint16_t dst_conn_id = item.first;

                Command cmd_disconnect;
                cmd_disconnect.buildCommand(CommandType::CMD_STOP, self_conn_id, dst_conn_id, INVALID_STRIPE_ID, INVALID_BLK_ID, INVALID_NODE_ID, INVALID_NODE_ID, string(), string());

                // add to cmd_dist_queue
                (*cmd_dist_queues)[dst_conn_id]->Push(cmd_disconnect);
            }
            skt.close();
            break;
        }
    }

    printf("CmdHandler::handleCmdFromController finished handling commands from Controller\n");
}

void CmdHandler::handleCmdFromAgent(uint16_t src_conn_id)
{
    printf("CmdHandler::handleCmdFromAgent start to handle commands from Agent %u\n", src_conn_id);

    // obtain the starter lock
    unique_lock<mutex> lck(ready_mtx);
    while (is_ready == false)
    {
        ready_cv.wait(lck, [&]
                      { return is_ready.load(); });
    }
    lck.unlock();

    unsigned char *cmd_handler_block_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char *));

    auto &skt = sockets_map[src_conn_id];

    while (true)
    {
        Command cmd;
        // retrieve command

        ssize_t ret_val = skt.read_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char));
        if (ret_val == -1)
        {
            fprintf(stderr, "CmdHandler::handleCmdFromAgent error reading command\n");
            exit(EXIT_FAILURE);
        }
        else if (ret_val == 0)
        {
            // currently, no cmd coming in
            printf("CmdHandler::handleCmdFromAgent no command coming in, break\n");
            break;
        }

        // parse the command
        cmd.parse();
        cmd.print();

        // validate command
        if (cmd.src_conn_id != src_conn_id)
        {
            fprintf(stderr, "CmdHandler::handleCmdFromAgent error: invalid command content\n");
            exit(EXIT_FAILURE);
        }

        // block transfer commands
        if (cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
        {
            // recv block
            if (BlockIO::recvBlock(skt, cmd_handler_block_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "CmdHandler::handleCmdFromAgent error recv block: %s\n", cmd.src_block_path.c_str());
                exit(EXIT_FAILURE);
            }

            printf("CmdHandler::handleCmdFromAgent received block transfer task and block: %s\n", cmd.dst_block_path.c_str());
            // Utils::printUCharBuffer(block_buffer, 10);

            if (cmd.type == CMD_TRANSFER_COMPUTE_BLK)
            { // parity block compute command

                // request a block from memory pool
                // unsigned char *block_buffer = memory_pool->getBlock();

                unsigned char *block_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

                memcpy(block_buffer, cmd_handler_block_buffer, config.block_size * sizeof(unsigned char));

                // pass to compute queue
                ParityComputeTask parity_compute_task(&config.code, cmd.post_stripe_id, cmd.post_block_id, block_buffer, cmd.dst_block_path);
                parity_compute_queue->Push(parity_compute_task);
            }
            else if (cmd.type == CMD_TRANSFER_RELOC_BLK)
            { // relocate command

                //  block
                if (BlockIO::writeBlock(cmd.dst_block_path, cmd_handler_block_buffer, config.block_size) != config.block_size)
                {
                    fprintf(stderr, "CmdHandler::handleCmdFromAgent error writing block: %s\n", cmd.dst_block_path.c_str());
                    exit(EXIT_FAILURE);
                }

                printf("CmdHandler::handleCmdFromAgent write block: %s\n", cmd.dst_block_path.c_str());
            }
        }
        else if (cmd.type == CommandType::CMD_STOP)
        { // check whether it's a stop command
            printf("CmdHandler::handleCmdFromAgent received stop command from Agent %u, call socket.close()\n", cmd.src_conn_id);
            skt.close();
            break;
        }
    }

    free(cmd_handler_block_buffer);

    printf("CmdHandler::handleCmdFromAgent finished handling commands for Agent %u\n", src_conn_id);
}
