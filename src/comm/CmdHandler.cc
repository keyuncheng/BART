#include "CmdHandler.hh"

CmdHandler::CmdHandler(Config &_config, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, MessageQueue<Command> *_cmd_dist_queue, MessageQueue<ParityComputeTask> *_parity_compute_queue, MemoryPool *_memory_pool, unsigned int _num_threads) : ThreadPool(_num_threads), config(_config), sockets_map(_sockets_map), cmd_dist_queue(_cmd_dist_queue), parity_compute_queue(_parity_compute_queue), memory_pool(_memory_pool)
{
    is_handler_ready = false;

    for (auto &item : sockets_map)
    {
        uint16_t conn_id = item.first;
        handler_threads_map[conn_id] = NULL;
    }
}

CmdHandler::~CmdHandler()
{
}

void CmdHandler::run()
{
    // init mutex
    unique_lock<mutex> lck(handler_mtx);
    is_handler_ready = true;
    lck.unlock();
    handler_cv.notify_all();

    for (auto &item : sockets_map)
    {
        uint16_t conn_id = item.first;

        // create cmd handler thread
        if (conn_id != CTRL_NODE_ID)
        {
            handler_threads_map[conn_id] = new thread(&CmdHandler::handleAgentCmd, this, conn_id);
        }
        else
        {
            handler_threads_map[conn_id] = new thread(&CmdHandler::handleControllerCmd, this);
        }
    }
}

void CmdHandler::waitForController()
{
    uint16_t conn_id = CTRL_NODE_ID;

    handler_threads_map[conn_id]->join();
    delete handler_threads_map[conn_id];
}

void CmdHandler::waitForAgents()
{
    // join the threads
    for (auto &item : sockets_map)
    {
        uint16_t conn_id = item.first;

        // create cmd handler thread
        if (conn_id != CTRL_NODE_ID)
        {
            handler_threads_map[conn_id]->join();
            delete handler_threads_map[conn_id];
        }
    }
}

void CmdHandler::handleControllerCmd()
{
    printf("CmdHandler:: start to handle commands from Controller\n");

    // obtain the starter lock
    unique_lock<mutex> lck(handler_mtx);
    while (is_handler_ready == false)
    {
        handler_cv.wait(lck, [&]
                        { return is_handler_ready.load(); });
    }
    lck.unlock();

    while (finished() == false)
    {
        auto &skt = sockets_map[CTRL_NODE_ID];

        Command cmd;
        // retrieve command
        auto ret_val = skt.read_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char));
        if (ret_val == -1)
        {
            fprintf(stderr, "error reading command\n");
            exit(EXIT_FAILURE);
        }
        else if (ret_val == 0)
        {
            // currently, no cmd comming in
            continue;
        }

        // parse the command
        cmd.parse();
        cmd.print();

        // validate command
        if (cmd.src_conn_id != CTRL_NODE_ID)
        {
            fprintf(stderr, "invalid command content\n");
            exit(EXIT_FAILURE);
        }

        // handle commands
        if (cmd.type == CommandType::CMD_LOCAL_COMPUTE_BLK)
        { // read block command
          // request a block from memory pool
            unsigned char *block_buffer = memory_pool->getBlock();

            // read block
            if (BlockIO::readBlock(cmd.src_block_path, block_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "error reading block: %s\n", cmd.src_block_path.c_str());
                exit(EXIT_FAILURE);
            }

            // pass to compute queue
            ParityComputeTask parity_compute_task(&config.code, cmd.post_stripe_id, cmd.post_block_id, block_buffer, cmd.dst_block_path);
            parity_compute_queue->Push(parity_compute_task);
        }
        else if (cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
        { // block transfer command
            // validate command
            if (cmd.src_node_id == cmd.dst_node_id)
            {
                fprintf(stderr, "invalid command content\n");
                exit(EXIT_FAILURE);
            }

            // forward the command to another node
            Command cmd_forward;

            cmd_forward.buildCommand(cmd.type, cmd.src_node_id, cmd.dst_node_id, cmd.post_block_id, cmd.post_block_id, cmd.src_node_id, cmd.dst_node_id, cmd.src_block_path, cmd.dst_block_path);

            printf("received block transfer task (type: %u, Node %u -> %u), forward block\n", cmd_forward.type, cmd_forward.src_node_id, cmd_forward.dst_node_id);

            // add to cmd_dist_queue
            cmd_dist_queue->Push(cmd_forward);
        }
        else if (cmd.type == CommandType::CMD_DELETE_BLK)
        { // handle delete block request
            BlockIO::deleteBlock(cmd.src_block_path);
        }
        else if (cmd.type == CMD_STOP)
        {
            printf("received stop command from Controller\n");
            skt.close();
            break;
        }
    }

    printf("CmdHandler:: handleControllerCmd exit\n");
}

void CmdHandler::handleAgentCmd(uint16_t conn_id)
{
    // printf("CmdHandler:: start to handle commands from Agent %u\n", conn_id);

    // // obtain the starter lock
    // unique_lock<mutex> lck(handler_mtx);
    // while (is_handler_ready == false)
    // {
    //     handler_cv.wait(lck, [&]
    //                     { return is_handler_ready.load(); });
    // }
    // lck.unlock();

    // while (finished() == false)
    // {
    //     auto &skt = sockets_map[conn_id];

    //     Command cmd;
    //     // retrieve command

    //     ssize_t ret_val = skt.read_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char));

    //     if (ret_val == -1)
    //     {
    //         fprintf(stderr, "error reading command\n");
    //         exit(EXIT_FAILURE);
    //     }
    //     else if (ret_val == 0)
    //     {
    //         // currently, no cmd comming in
    //         continue;
    //     }

    //     printf("I'm here too too\n");

    //     // parse the command
    //     cmd.parse();
    //     cmd.print();

    //     // check whether it's a stop command
    //     if (cmd.type == CommandType::CMD_STOP)
    //     {
    //         printf("received stop command from Agent %u\n", cmd.src_conn_id);
    //         skt.close();
    //         break;
    //     }

    //     // the other commands are transfer commands

    //     // validate command
    //     if (cmd.src_conn_id != conn_id || (cmd.type != CommandType::CMD_TRANSFER_COMPUTE_BLK && cmd.type != CommandType::CMD_TRANSFER_RELOC_BLK) || cmd.src_node_id == cmd.dst_node_id)
    //     {
    //         fprintf(stderr, "invalid command content\n");
    //         exit(EXIT_FAILURE);
    //     }

    //     // request a block from memory pool
    //     unsigned char *block_buffer = memory_pool->getBlock();

    //     // recv block
    //     if (BlockIO::recvBlock(skt, block_buffer, config.block_size) != config.block_size)
    //     {
    //         fprintf(stderr, "invalid block: %s\n", cmd.src_block_path.c_str());
    //         exit(EXIT_FAILURE);
    //     }

    //     printf("received block transfer task and block\n");
    //     // Utils::printUCharBuffer(block_buffer, 10);

    //     if (cmd.type == CMD_TRANSFER_COMPUTE_BLK)
    //     { // parity block compute command

    //         // pass to compute queue
    //         ParityComputeTask parity_compute_task(&config.code, cmd.post_stripe_id, cmd.post_block_id, block_buffer, cmd.dst_block_path);
    //         parity_compute_queue->Push(parity_compute_task);
    //     }
    //     else if (cmd.type == CMD_TRANSFER_RELOC_BLK)
    //     { // relocate command

    //         //  block
    //         if (BlockIO::writeBlock(cmd.dst_block_path, block_buffer, config.block_size) != config.block_size)
    //         {
    //             fprintf(stderr, "error writing block: %s\n", cmd.dst_block_path.c_str());
    //             exit(EXIT_FAILURE);
    //         }

    //         // free block
    //         memory_pool->freeBlock(block_buffer);
    //     }
    // }

    // printf("CmdHandler:: handleAgentCmd exit\n");
}
