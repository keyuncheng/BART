#include "CmdHandler.hh"

CmdHandler::CmdHandler(Config &_config, uint16_t _self_conn_id,
                       unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map,
                       unordered_map<uint16_t, MessageQueue<Command> *> *_cmd_dist_queues,
                       unordered_map<unsigned int, MessageQueue<ParityComputeTask> *> *_compute_task_queues,
                       unordered_map<unsigned int, MultiWriterQueue<Command> *> *_reloc_task_queues) : ThreadPool(1), config(_config), self_conn_id(_self_conn_id), sockets_map(_sockets_map), cmd_dist_queues(_cmd_dist_queues), compute_task_queues(_compute_task_queues), reloc_task_queues(_reloc_task_queues)
{
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
    // create cmd handler thread for Controller and Agents
    for (auto &item : handler_threads_map)
    {
        uint16_t conn_id = item.first;
        if (conn_id == CTRL_NODE_ID)
        {
            item.second = new thread(&CmdHandler::handleCmdFromController, this);
        }
        else
        {
            item.second = new thread(&CmdHandler::handleCmdFromAgent, this, conn_id);
        }
    }

    // distribute stop commands (from Agents only)
    if (self_conn_id != CTRL_NODE_ID)
    {
        distStopCmds();
    }

    // join cmd handler thread for both Controller and Agents
    for (auto &item : handler_threads_map)
    {
        thread *cmd_handler_thd = item.second;
        cmd_handler_thd->join();
        delete cmd_handler_thd;
    }
}

void CmdHandler::handleCmdFromController()
{
    printf("[Node %u] CmdHandler::handleCmdFromController start to handle commands from Controller\n", self_conn_id);

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

        // cmd.print();
        // printf("CmdHandler::handleCmdFromController handle command, type: %u, (%u -> %u), post: (%u, %u)\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id, cmd.post_stripe_id, cmd.post_block_id);

        // validate command
        if (cmd.src_conn_id != src_conn_id || cmd.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid command content\n");
            exit(EXIT_FAILURE);
        }

        if (cmd.type == CommandType::CMD_COMPUTE_RE_BLK || cmd.type == CommandType::CMD_COMPUTE_PM_BLK)
        { // parity block compute task

            // validate command
            if (self_conn_id != cmd.src_node_id || cmd.src_node_id != cmd.dst_node_id)
            {
                fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid compute command content\n");
                exit(EXIT_FAILURE);
            }

            // parse to a parity compute task
            ParityComputeTask parity_compute_task(&config.code, cmd.post_stripe_id, cmd.post_block_id, cmd.enc_method, cmd.src_block_nodes, cmd.parity_reloc_nodes, cmd.src_block_path, cmd.dst_block_path);

            // push the compute task to specific compute task queue
            unsigned int assigned_worker_id = parity_compute_task.post_stripe_id % config.num_compute_workers;
            (*compute_task_queues)[assigned_worker_id]->Push(parity_compute_task);

            printf("CmdHandler::handleCmdFromController received parity computation task, forward to ComputeWorker %u, post: (%u, %u), enc_method: %u\n", assigned_worker_id, parity_compute_task.post_stripe_id, parity_compute_task.post_block_id, parity_compute_task.enc_method);
        }
        else if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
        { // relocate block task

            // only parse for data block relocation; parity block relocation will be generated by ComputeWorker
            if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK && cmd.post_block_id >= config.code.k_f)
            {
                fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid transfer command content\n");
                exit(EXIT_FAILURE);
            }

            // forward the command to another node
            Command cmd_reloc;
            cmd_reloc.buildCommand(cmd.type, self_conn_id, cmd.dst_node_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_node_id, cmd.dst_node_id, cmd.src_block_path, cmd.dst_block_path);

            // push the relocation task to specific relocation task queue
            unsigned int assigned_worker_id = cmd_reloc.post_stripe_id % config.num_reloc_workers;
            // (*reloc_task_queues)[assigned_worker_id]->Push(cmd_reloc);

            printf("CmdHandler::handleCmdFromAgent received relocation task, forward to RelocWorker %u, post: (%u, %u)\n", assigned_worker_id, cmd_reloc.post_stripe_id, cmd_reloc.post_block_id);
        }
        else if (cmd.type == CommandType::CMD_DELETE_BLK)
        { // delete block task
            BlockIO::deleteBlock(cmd.src_block_path);
        }
        else if (cmd.type == CMD_STOP)
        { // stop task
            // push stop tasks to compute queues
            ParityComputeTask term_task(true);
            for (auto item : *compute_task_queues)
            {
                auto &compute_task_queue = item.second;
                compute_task_queue->Push(term_task);
            }

            // push stop tasks to reloc queues
            Command stop_cmd;
            stop_cmd.buildCommand(CommandType::CMD_STOP, INVALID_NODE_ID, INVALID_NODE_ID);
            for (auto item : *reloc_task_queues)
            {
                auto &reloc_task_queue = item.second;
                reloc_task_queue->Push(stop_cmd);
            }

            // close socket
            skt.close();

            // printf("CmdHandler::handleCmdFromController received stop command from Controller\n");
            break;
        }
    }

    printf("[Node %u] CmdHandler::handleCmdFromController finished handling commands from Controller\n", self_conn_id);
}

void CmdHandler::handleCmdFromAgent(uint16_t src_conn_id)
{
    printf("[Node %u] CmdHandler::handleCmdFromAgent start to handle commands from Agent %u\n", self_conn_id, src_conn_id);

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

        // cmd.print();
        // printf("CmdHandler::handleCmdFromAgent handle command, type: %u, (%u -> %u), post: (%u, %u)\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id, cmd.post_stripe_id, cmd.post_block_id);

        // validate command
        if (cmd.src_conn_id != src_conn_id || cmd.dst_conn_id != self_conn_id)
        {
            fprintf(stderr, "CmdHandler::handleCmdFromAgent error: invalid command content\n");
            exit(EXIT_FAILURE);
        }

        if (cmd.type == CommandType::CMD_STOP)
        {
            // printf("CmdHandler::handleCmdFromAgent received stop command from Agent %u\n", cmd.src_conn_id);
            skt.close();
            break;
        }
    }

    printf("[Node %u] CmdHandler::handleCmdFromAgent finished handling commands for Agent %u\n", self_conn_id, src_conn_id);
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