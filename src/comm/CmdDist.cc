#include "CmdDist.hh"

CmdDist::CmdDist(Config &_config, uint16_t _self_conn_id, unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, MessageQueue<Command> *> &_cmd_dist_queues) : ThreadPool(1), config(_config), self_conn_id(_self_conn_id), connectors_map(_connectors_map), cmd_dist_queues(_cmd_dist_queues)
{
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        dist_threads_map[conn_id] = NULL;
    }
}

CmdDist::~CmdDist()
{
}

void CmdDist::run()
{
    // create cmd dist thread
    for (auto &item : dist_threads_map)
    {
        uint16_t conn_id = item.first;
        item.second = new thread(&CmdDist::distCmdToNode, this, conn_id);
    }

    // join cmd dist thread
    for (auto &item : dist_threads_map)
    {
        thread *cmd_dist_thd = item.second;
        cmd_dist_thd->join();
        delete cmd_dist_thd;
    }
}

void CmdDist::distCmdToNode(uint16_t dst_conn_id)
{
    printf("[Node %u] CmdDist::distCmdToNode start to distribute commands to Node %u\n", self_conn_id, dst_conn_id);

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
                fprintf(stderr, "CmdDist::distControllerCmd error: invalid command content: %u, %u\n", cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            // cmd.print();
            // printf("CmdHandler::distControllerCmd get command, type: %u, (%u -> %u), post: (%u, %u)\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id, cmd.post_stripe_id, cmd.post_block_id);

            // send the command
            if (connector.write_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "CmdDist::distCmdToNode error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
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

    printf("[Node %u] CmdDist::distCmdToNode finished distributing commands to Node %u\n", self_conn_id, dst_conn_id);
}