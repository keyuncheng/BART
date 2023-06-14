#include "CmdHandler.hh"

CmdHandler::CmdHandler(Config &_config, uint16_t _self_conn_id, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, unordered_map<uint16_t, MessageQueue<Command> *> *_cmd_dist_queues, MultiWriterQueue<ParityComputeTask> *_parity_compute_queue, MessageQueue<string> *_parity_comp_result_queue, MemoryPool *_memory_pool, ComputeWorker *_compute_worker, unsigned int _num_threads) : ThreadPool(_num_threads), config(_config), self_conn_id(_self_conn_id), sockets_map(_sockets_map), cmd_dist_queues(_cmd_dist_queues), parity_compute_queue(_parity_compute_queue), parity_comp_result_queue(_parity_comp_result_queue), memory_pool(_memory_pool), compute_worker(_compute_worker)
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

    if (self_conn_id != CTRL_NODE_ID)
    {
        // create cmd handler thread from Controller
        handler_threads_map[CTRL_NODE_ID] = new thread(&CmdHandler::handleCmdFromController, this);

        // join cmd handler thread from Controller
        handler_threads_map[CTRL_NODE_ID]->join();
        delete handler_threads_map[CTRL_NODE_ID];
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

    // collect parity computation results thread
    if (self_conn_id != CTRL_NODE_ID)
    {
        // create parity relocation command thread
        parity_reloc_thread = new thread(&CmdHandler::handleParityRelocCmdFromController, this);

        parity_reloc_thread->join();
        delete parity_reloc_thread;
    }

    // join cmd handler thread for Agent
    for (auto &item : sockets_map)
    {
        uint16_t conn_id = item.first;
        if (conn_id != CTRL_NODE_ID)
        {
            handler_threads_map[conn_id]->join();
            delete handler_threads_map[conn_id];
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

        // handle commands
        if (cmd.type == CommandType::CMD_COMPUTE_PM_BLK || cmd.type == CommandType::CMD_COMPUTE_RE_BLK)
        {
            // validate command
            if (cmd.src_node_id != cmd.dst_node_id)
            {
                fprintf(stderr, "CmdHandler::handleCmdFromController error: invalid local compute command content\n");
                exit(EXIT_FAILURE);
            }

            // pass to compute queue (for creating a record for computation)
            EncodeMethod enc_method = (cmd.type == CommandType::CMD_COMPUTE_RE_BLK) ? EncodeMethod::RE_ENCODE : EncodeMethod::PARITY_MERGE;
            ParityComputeTask parity_compute_task(&config.code, cmd.post_stripe_id, cmd.post_block_id, enc_method, NULL, cmd.dst_block_path);

            parity_compute_queue->Push(parity_compute_task);

            printf("CmdHandler::handleCmdFromController create compute task for parity compute: (%u, %u), enc_method: %u\n", cmd.post_stripe_id, cmd.post_block_id, enc_method);
        }
        else if (cmd.type == CommandType::CMD_READ_COMPUTE_BLK)
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
            EncodeMethod enc_method = (cmd.post_block_id < config.code.k_f) ? EncodeMethod::RE_ENCODE : EncodeMethod::PARITY_MERGE;
            ParityComputeTask parity_compute_task(&config.code, cmd.post_stripe_id, cmd.post_block_id, enc_method, block_buffer, cmd.dst_block_path);
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

            // add lock (to dst node)
            uint16_t dst_conn_id = cmd.dst_node_id;

            // forward the command to another node
            Command cmd_forward;

            cmd_forward.buildCommand(cmd.type, self_conn_id, dst_conn_id, cmd.post_stripe_id, cmd.post_block_id, cmd.src_node_id, cmd.dst_node_id, cmd.src_block_path, cmd.dst_block_path);

            // special handling for parity block relocation task
            // need to wait for parity compute queue to finish
            if (cmd_forward.type == CommandType::CMD_TRANSFER_RELOC_BLK && cmd_forward.post_block_id >= config.code.k_f)
            {
                // continue;

                // create a parity reloc record, to aggregate the blocks to relocate
                lock_guard<mutex> lck(parity_reloc_cmd_map_mtx);
                if (parity_reloc_cmd_map.find(cmd_forward.post_stripe_id) == parity_reloc_cmd_map.end())
                {
                    parity_reloc_cmd_map.insert(pair<uint32_t, vector<Command>>(cmd_forward.post_stripe_id, vector<Command>(config.code.m_f, Command())));
                }

                // associate the block with the command
                parity_reloc_cmd_map[cmd_forward.post_stripe_id][cmd_forward.post_block_id - config.code.k_f] = cmd_forward;

                printf("handleCmdFromController:: received parity block relocation task (type: %u, post_stripe: (%u, %u), Node %u -> %u), cached\n", cmd_forward.type, cmd_forward.post_stripe_id, cmd_forward.post_block_id, cmd_forward.src_node_id, cmd_forward.dst_node_id);

                continue;
            }

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
            // pass to parity compute thread to terminate
            ParityComputeTask parity_compute_term_task(&config.code, INVALID_STRIPE_ID_GLOBAL, INVALID_BLK_ID, EncodeMethod::UNKNOWN_METHOD, NULL, string());
            parity_compute_term_task.all_tasks_finished = true;
            parity_compute_queue->Push(parity_compute_term_task);

            // close socket
            skt.close();

            printf("CmdHandler::handleCmdFromController received stop command from Controller, call socket.close()\n");

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
                EncodeMethod enc_method = (cmd.post_block_id < config.code.k_f) ? EncodeMethod::RE_ENCODE : EncodeMethod::PARITY_MERGE;
                ParityComputeTask parity_compute_task(&config.code, cmd.post_stripe_id, cmd.post_block_id, enc_method, block_buffer, cmd.dst_block_path);
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

void CmdHandler::handleParityRelocCmdFromController()
{
    printf("CmdHandler::handleParityRelocCmdFromController start to handle parity relocation commands from Controller\n");

    string parity_compute_key;
    string parity_reloc_key;
    EncodeMethod enc_method = UNKNOWN_METHOD;
    uint32_t post_stripe_id = INVALID_STRIPE_ID_GLOBAL;
    uint8_t post_block_id = INVALID_BLK_ID;

    bool is_finished = false;

    while (true)
    {
        if (parity_reloc_cmd_map.empty() == true && parity_comp_result_queue->IsEmpty() == true && is_finished == true)
        {
            break;
        }

        if (parity_comp_result_queue->Pop(parity_compute_key) == true)
        {
            if (parity_compute_key.compare(parity_comp_term_signal) == 0)
            {
                is_finished = true;
                continue;
            }

            // obtain the parity compute key
            ComputeWorker::getParityComputeTaskFromKey(parity_compute_key, enc_method, post_stripe_id, post_block_id);

            printf("CmdHandler::handleParityRelocCmdFromController obtained key: %s, post_stripe: (%u, %u), enc_method: %u\n", parity_compute_key.c_str(), post_stripe_id, post_block_id, enc_method);

            // check whether the parity blocks need to be relocated
            if (parity_reloc_cmd_map.find(post_stripe_id) == parity_reloc_cmd_map.end())
            {
                continue;
            }

            // check from parity reloc command
            vector<uint8_t> parity_ids_to_reloc;

            if (enc_method == EncodeMethod::RE_ENCODE)
            {
                // relocate every parity block
                for (uint8_t parity_id = 0; parity_id < config.code.m_f; parity_id++)
                {
                    Command &cmd_forward = parity_reloc_cmd_map[post_stripe_id][parity_id];

                    if (cmd_forward.type != CommandType::CMD_UNKNOWN)
                    {
                        parity_ids_to_reloc.push_back(parity_id);
                    }
                }
            }
            else if (enc_method == EncodeMethod::PARITY_MERGE)
            {
                uint8_t parity_id = post_block_id - config.code.k_f;
                Command &cmd_forward = parity_reloc_cmd_map[post_stripe_id][parity_id];
                if (cmd_forward.type != CommandType::CMD_UNKNOWN)
                {
                    parity_ids_to_reloc.push_back(parity_id);
                }
            }

            for (auto parity_id : parity_ids_to_reloc)
            {
                Command &cmd_forward = parity_reloc_cmd_map[post_stripe_id][parity_id];

                // add to cmd_dist_queue[dst_node_id]
                (*cmd_dist_queues)[cmd_forward.dst_node_id]->Push(cmd_forward);

                // clear out the record
                parity_reloc_cmd_map[post_stripe_id][parity_id] = Command();

                printf("CmdHandler::handleParityRelocCmdFromController handle parity block relocation task (type: %u, Node %u -> %u), forward block: (%u, %u), enc_method: %u\n", cmd_forward.type, cmd_forward.src_node_id, cmd_forward.dst_node_id, cmd_forward.post_stripe_id, cmd_forward.post_block_id, enc_method);
            }

            // needed to erase the record
            bool needed_to_erase_record = true;
            // erase the record
            for (uint8_t parity_id = 0; parity_id < config.code.m_f; parity_id++)
            {
                Command &cmd_forward = parity_reloc_cmd_map[post_stripe_id][parity_id];

                if (cmd_forward.type != CommandType::CMD_UNKNOWN)
                {
                    needed_to_erase_record = false;
                    break;
                }
            }

            if (needed_to_erase_record)
            {
                lock_guard<mutex> lck(parity_reloc_cmd_map_mtx);
                parity_reloc_cmd_map.erase(post_stripe_id);
            }
        }
    }

    // distribute terminate commands
    for (auto &item : sockets_map)
    {
        uint16_t dst_conn_id = item.first;

        Command cmd_disconnect;
        cmd_disconnect.buildCommand(CommandType::CMD_STOP, self_conn_id, dst_conn_id, INVALID_STRIPE_ID, INVALID_BLK_ID, INVALID_NODE_ID, INVALID_NODE_ID, string(), string());

        // add to cmd_dist_queue
        (*cmd_dist_queues)[dst_conn_id]->Push(cmd_disconnect);
    }

    printf("CmdHandler::handleParityRelocCmdFromController finished handling parity relocation commands from Controller\n");
}