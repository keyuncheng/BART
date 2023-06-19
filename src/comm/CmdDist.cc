#include "CmdDist.hh"

CmdDist::CmdDist(Config &_config, uint16_t _self_conn_id, unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, MessageQueue<Command> *> &_cmd_dist_queues, MemoryPool *_memory_pool, unsigned int _num_threads) : ThreadPool(_num_threads), config(_config), self_conn_id(_self_conn_id), connectors_map(_connectors_map), cmd_dist_queues(_cmd_dist_queues), memory_pool(_memory_pool)
{
    // init mutex
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        dist_threads_map[conn_id] = NULL;
    }

    is_ready = false;
}

CmdDist::~CmdDist()
{
}

void CmdDist::run()
{
    // init ready mutex
    unique_lock<mutex> lck(ready_mtx);
    is_ready = true;
    lck.unlock();
    ready_cv.notify_all();

    // create cmd dist thread
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;

        if (conn_id != CTRL_NODE_ID)
        { // create cmd dist thread for Agents
            dist_threads_map[conn_id] = new thread(&CmdDist::distCmdToAgent, this, conn_id);
        }
        else
        {
            dist_threads_map[conn_id] = new thread(&CmdDist::distCmdToController, this);
        }
    }

    // join cmd dist thread
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;

        dist_threads_map[conn_id]->join();
        delete dist_threads_map[conn_id];
    }
}

void CmdDist::distCmdToController()
{
    printf("CmdDist::distCmdToController [Node %u] start to distribute commands to Controller\n", self_conn_id);

    // obtain the starter lock
    unique_lock<mutex> lck(ready_mtx);
    while (is_ready == false)
    {
        ready_cv.wait(lck, [&]
                      { return is_ready.load(); });
    }
    lck.unlock();

    uint16_t dst_conn_id = CTRL_NODE_ID;
    auto &connector = connectors_map[dst_conn_id];
    MessageQueue<Command> &cmd_dist_queue = *cmd_dist_queues[dst_conn_id];

    bool is_finished = false;

    while (true)
    {
        if (cmd_dist_queue.IsEmpty() == true && is_finished == true)
        {
            break;
        }

        Command cmd;
        if (cmd_dist_queue.Pop(cmd) == true)
        {
            // validate
            if (cmd.src_conn_id != self_conn_id || cmd.dst_conn_id != dst_conn_id)
            {
                fprintf(stderr, "CmdDist::distCmdToController error: invalid command content: %u, %u\n", cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            cmd.print();

            // // add lock (to dst node)
            // lock_guard<mutex> lck(*dist_mtxs_map[cmd.dst_conn_id]);

            // send the command
            if (connector.write_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "CmdDist::distCmdToController:: error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            if (cmd.type == CommandType::CMD_STOP)
            { // stop connection command
                // close the connector
                connector.close();
                is_finished = true;
            }
        }
    }

    printf("CmdDist::distCmdToController [Node %u] finished distribute commands to Controller\n", self_conn_id);
}

void CmdDist::distCmdToAgent(uint16_t dst_conn_id)
{
    printf("CmdDist::distCmdToAgent [Node %u] start to distribute commands to Agent %u\n", self_conn_id, dst_conn_id);

    // obtain the starter lock
    unique_lock<mutex> lck(ready_mtx);
    while (is_ready == false)
    {
        ready_cv.wait(lck, [&]
                      { return is_ready.load(); });
    }
    lck.unlock();

    auto &connector = connectors_map[dst_conn_id];
    MessageQueue<Command> &cmd_dist_queue = *cmd_dist_queues[dst_conn_id];
    bool is_finished = false;

    // allocate block buffer
    unsigned char *local_block_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

    while (true)
    {
        if (cmd_dist_queue.IsEmpty() == true && is_finished == true)
        {
            break;
        }

        Command cmd;
        if (cmd_dist_queue.Pop(cmd) == true)
        {
            // validate
            if (cmd.src_conn_id != self_conn_id || cmd.dst_conn_id != dst_conn_id)
            {
                fprintf(stderr, "CmdDist::distControllerCmd error: invalid command content: %u, %u\n", cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            cmd.print();

            // send the command
            if (connector.write_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "CmdDist::distCmdToAgent error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            // For block transfer command (sent from Agent only)
            if (self_conn_id != CTRL_NODE_ID)
            {
                if (cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
                { // after sending the command to Agent, we send the corresponding block

                    // read block
                    if (BlockIO::readBlock(cmd.src_block_path, local_block_buffer, config.block_size) != config.block_size)
                    {
                        fprintf(stderr, "CmdDist::distCmdToAgent error reading block: %s\n", cmd.src_block_path.c_str());
                        exit(EXIT_FAILURE);
                    }

                    // send block
                    // hcpuyang: separate the connector, should be used as dedicated for block transfer
                    if (BlockIO::sendBlock(connector, local_block_buffer, config.block_size) != config.block_size)
                    {
                        fprintf(stderr, "CmdDist::distCmdToAgent error sending block: %s to Node %u\n", cmd.src_block_path.c_str(), cmd.dst_conn_id);
                        exit(EXIT_FAILURE);
                    }

                    printf("CmdDist::distCmdToAgent send block to Node %u, block_path: %s\n", dst_conn_id, cmd.src_block_path.c_str());
                    // Utils::printUCharBuffer(block_buffer, 10);
                }
            }

            if (cmd.type == CommandType::CMD_STOP)
            { // stop connection command
                // close the connector
                connector.close();
                is_finished = true;
            }
        }
    }

    // free block buffer
    free(local_block_buffer);

    printf("CmdDist::distCmdToAgent [Node %u] finished distribute commands to Agent %u\n", self_conn_id, dst_conn_id);
}