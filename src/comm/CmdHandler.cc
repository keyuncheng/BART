#include "CmdHandler.hh"

CmdHandler::CmdHandler(Config &_config, uint16_t _self_conn_id, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, unordered_map<uint16_t, MessageQueue<Command> *> *_cmd_dist_queues, unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> *_pc_task_queues,
                       unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> *_pc_reply_queues, MessageQueue<ParityComputeTask> *_parity_reloc_task_queue, MemoryPool *_memory_pool, unsigned int _num_threads) : ThreadPool(_num_threads), config(_config), self_conn_id(_self_conn_id), sockets_map(_sockets_map), cmd_dist_queues(_cmd_dist_queues), pc_task_queues(_pc_task_queues), pc_reply_queues(_pc_reply_queues), parity_reloc_task_queue(_parity_reloc_task_queue), memory_pool(_memory_pool)
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

    // create parity relocation thread
    if (self_conn_id != CTRL_NODE_ID)
    {
        parity_reloc_task_thread = new thread(&CmdHandler::handleParityRelocTask, this);
    }

    // create cmd handler thread for Controller and Agents
    for (auto &item : sockets_map)
    {
        uint16_t conn_id = item.first;

        if (conn_id != CTRL_NODE_ID)
        { // create handler thread for Agents
            handler_threads_map[conn_id] = new thread(&CmdHandler::handleCmdFromAgent, this, conn_id);
        }
        else
        {
            handler_threads_map[conn_id] = new thread(&CmdHandler::handleCmdFromController, this);
        }
    }

    if (self_conn_id != CTRL_NODE_ID)
    {
        // join parity relocation thread
        parity_reloc_task_thread->join();

        // distribute stop commands
        distStopCmds();
    }

    // join cmd handler thread for both Controller and Agents
    for (auto &item : sockets_map)
    {
        uint16_t conn_id = item.first;
        handler_threads_map[conn_id]->join();
        delete handler_threads_map[conn_id];
    }
}

void CmdHandler::handleCmdFromController()
{
    printf("CmdHandler::handleCmdFromController [Node %u] start to handle commands from Controller\n", self_conn_id);

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

    MessageQueue<ParityComputeTask> &pc_controller_task_queue = *((*pc_task_queues)[CTRL_NODE_ID]);

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
        if (cmd.src_conn_id != src_conn_id || cmd.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid command content\n");
            exit(EXIT_FAILURE);
        }

        if (cmd.type == CommandType::CMD_COMPUTE_RE_BLK || cmd.type == CommandType::CMD_COMPUTE_PM_BLK)
        {
            // validate command
            if (self_conn_id != cmd.src_node_id || cmd.src_node_id != cmd.dst_node_id)
            {
                fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid compute command content\n");
                exit(EXIT_FAILURE);
            }

            // parse to a command
            ParityComputeTask parity_compute_task(&config.code, cmd.post_stripe_id, cmd.post_block_id, cmd.src_node_id, cmd.enc_method, cmd.src_block_nodes, cmd.parity_reloc_nodes, cmd.src_block_path, cmd.dst_block_path);

            // push a command to parity compute queue (controller)
            pc_controller_task_queue.Push(parity_compute_task);

            printf("CmdHandler::handleCmdFromController received parity computation task, forward to ComputeWorker, post: (%u, %u), enc_method: %u\n", parity_compute_task.post_stripe_id, parity_compute_task.post_block_id, parity_compute_task.enc_method);
        }
        else if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
        { // block transfer command

            uint16_t dst_conn_id = cmd.dst_node_id;

            // validate command
            if (cmd.src_node_id == dst_conn_id)
            {
                fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid transfer command content\n");
                exit(EXIT_FAILURE);
            }

            // only parse for data block relocation; the parity block relocation will be generated by ComputeWorker
            if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK && cmd.post_block_id >= config.code.k_f)
            {
                fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid transfer command content\n");
                exit(EXIT_FAILURE);
            }

            // forward the command to another node
            cmd.print();

            Command cmd_forward;
            cmd_forward.buildCommand(cmd.type, self_conn_id, dst_conn_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_node_id, dst_conn_id, cmd.src_block_path, cmd.dst_block_path);

            // add to cmd_dist_queue[dst_node_id]
            (*cmd_dist_queues)[dst_conn_id]->Push(cmd_forward);

            printf("CmdHandler::handleCmdFromController received block transfer task (type: %u, Node %u -> %u), forwarded to CmdDist\n", cmd_forward.type, cmd_forward.src_node_id, cmd_forward.dst_node_id);
        }
        else if (cmd.type == CommandType::CMD_DELETE_BLK)
        { // handle delete block request
            BlockIO::deleteBlock(cmd.src_block_path);
        }
        else if (cmd.type == CMD_STOP)
        {
            // parse to a command
            ParityComputeTask term_task(true);

            // push a command to parity compute queue (controller)
            MessageQueue<ParityComputeTask> &pc_controller_task_queue = *((*pc_task_queues)[CTRL_NODE_ID]);
            pc_controller_task_queue.Push(term_task);

            // close socket
            skt.close();

            printf("CmdHandler::handleCmdFromController received stop command from Controller, call socket.close()\n");
            break;
        }
    }

    printf("CmdHandler::handleCmdFromController [Node %u] finished handling commands from Controller\n", self_conn_id);
}

void CmdHandler::handleCmdFromAgent(uint16_t src_conn_id)
{
    printf("CmdHandler::handleCmdFromAgent [Node %u] start to handle commands from Agent %u\n", self_conn_id, src_conn_id);

    // obtain the starter lock
    unique_lock<mutex> lck(ready_mtx);
    while (is_ready == false)
    {
        ready_cv.wait(lck, [&]
                      { return is_ready.load(); });
    }
    lck.unlock();

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
        if (cmd.src_conn_id != src_conn_id || cmd.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "CmdHandler::handleCmdFromAgent error: invalid command content\n");
            exit(EXIT_FAILURE);
        }
        else if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
        {
            printf("CmdHandler::handleCmdFromAgent received block relocation task: %s\n", cmd.dst_block_path.c_str());

            // // recv block
            // if (BlockIO::recvBlock(skt, cmd_handler_block_buffer, config.block_size) != config.block_size)
            // {
            //     fprintf(stderr, "CmdHandler::handleCmdFromAgent error recv block: %s\n", cmd.src_block_path.c_str());
            //     exit(EXIT_FAILURE);
            // }

            // //  block
            // if (BlockIO::writeBlock(cmd.dst_block_path, cmd_handler_block_buffer, config.block_size) != config.block_size)
            // {
            //     fprintf(stderr, "CmdHandler::handleCmdFromAgent error writing block: %s\n", cmd.dst_block_path.c_str());
            //     exit(EXIT_FAILURE);
            // }

            printf("CmdHandler::handleCmdFromAgent \n\n\n\n\n\n I didn't write block: %s\n\n\n\n\n\n", cmd.dst_block_path.c_str());
        }
        else if (cmd.type == CommandType::CMD_STOP)
        {
            printf("CmdHandler::handleCmdFromAgent received stop command from Agent %u, call socket.close()\n", cmd.src_conn_id);
            skt.close();
            break;
        }
    }

    printf("CmdHandler::handleCmdFromAgent [Node %u] finished handling commands for Agent %u\n", self_conn_id, src_conn_id);
}

void CmdHandler::handleParityRelocTask()
{
    printf("CmdHandler::handleParityRelocTask [Node %u] start to handle parity relocation task\n", self_conn_id);

    ParityComputeTask parity_reloc_task;
    bool is_finished = false;

    while (true)
    {
        if (parity_reloc_task_queue->IsEmpty() == true && is_finished == true)
        {
            break;
        }

        if (parity_reloc_task_queue->Pop(parity_reloc_task) == true)
        {
            // check parity relocation info
            if (parity_reloc_task.all_tasks_finished == true)
            {
                is_finished = true;
                continue;
            }

            if (parity_reloc_task.enc_method == EncodeMethod::RE_ENCODE)
            {
                for (uint8_t parity_id = 0; parity_id < config.code.m_f; parity_id++)
                {
                    uint16_t dst_conn_id = parity_reloc_task.parity_reloc_nodes[parity_id];
                    string &dst_block_path = parity_reloc_task.parity_block_dst_paths[parity_id];

                    // check whether needs relocation
                    if (self_conn_id != dst_conn_id)
                    {
                        Command cmd_reloc;
                        cmd_reloc.buildCommand(CommandType::CMD_TRANSFER_RELOC_BLK, self_conn_id, dst_conn_id, parity_reloc_task.post_stripe_id, parity_reloc_task.post_block_id, self_conn_id, dst_conn_id, dst_block_path, dst_block_path);

                        // add to cmd_dist_queue[dst_node_id]
                        (*cmd_dist_queues)[dst_conn_id]->Push(cmd_reloc);

                        printf("CmdHandler::handleCmdFromController created block reloc task (type: %u, Node %u -> %u), forwarded to CmdDist\n", cmd_reloc.type, cmd_reloc.src_node_id, cmd_reloc.dst_node_id);
                    }
                }
            }
            else if (parity_reloc_task.enc_method == EncodeMethod::PARITY_MERGE)
            {
                // send only one block
                uint16_t dst_conn_id = parity_reloc_task.parity_reloc_nodes[0];
                string &dst_block_path = parity_reloc_task.parity_block_dst_paths[0];

                Command cmd_reloc;
                cmd_reloc.buildCommand(CommandType::CMD_TRANSFER_RELOC_BLK, self_conn_id, dst_conn_id, parity_reloc_task.post_stripe_id, parity_reloc_task.post_block_id, self_conn_id, dst_conn_id, dst_block_path, dst_block_path);

                // add to cmd_dist_queue[dst_node_id]
                (*cmd_dist_queues)[dst_conn_id]->Push(cmd_reloc);

                printf("CmdHandler::handleCmdFromController created block reloc task (type: %u, Node %u -> %u), forwarded to CmdDist\n", cmd_reloc.type, cmd_reloc.src_node_id, cmd_reloc.dst_node_id);
            }
        }
    }

    printf("CmdHandler::handleParityRelocTask [Node %u] end handling parity relocation task\n", self_conn_id);
}

void CmdHandler::distStopCmds()
{
    // distribute terminate commands
    for (auto &item : sockets_map)
    {
        uint16_t dst_conn_id = item.first;

        Command cmd_disconnect;
        cmd_disconnect.buildCommand(CommandType::CMD_STOP, self_conn_id, dst_conn_id);

        // add to cmd_dist_queue
        (*cmd_dist_queues)[dst_conn_id]->Push(cmd_disconnect);
    }
}