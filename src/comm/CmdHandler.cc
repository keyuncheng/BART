#include "CmdHandler.hh"

CmdHandler::CmdHandler(unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, sockpp::tcp_acceptor &_acceptor, MessageQueue<Command> &_cmd_dist_queue, unsigned int _num_threads) : ThreadPool(_num_threads), connectors_map(_connectors_map), sockets_map(_sockets_map), acceptor(_acceptor), cmd_dist_queue(_cmd_dist_queue)
{
    // init mutex
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        mtxs_map[conn_id] = new mutex();
    }
}

CmdHandler::~CmdHandler()
{
    // init mutex
    for (auto &item : mtxs_map)
    {
        delete mtxs_map[item.first];
    }
}

void CmdHandler::run()
{
    // init mutex
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;

        // create cmd handler thread
        if (conn_id == CTRL_NODE_ID)
        {
            handler_threads_map[conn_id] = new thread(&CmdHandler::handleControllerCmd, this);
        }
        else
        {
            handler_threads_map[conn_id] = new thread(&CmdHandler::handleAgentCmd, this, conn_id);
        }
    }
}

void CmdHandler::handleControllerCmd()
{
    printf("start to handle Controller commands\n");

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
        if (cmd.src_conn_id != CTRL_NODE_ID || cmd.dst_conn_id != cmd.src_node_id)
        {
            fprintf(stderr, "invalid command content\n");
            exit(EXIT_FAILURE);
        }

        // forward transfer block command
        if (cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
        {
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
            cmd_dist_queue.Push(cmd_forward);
        }
    }
}

void CmdHandler::handleAgentCmd(uint16_t conn_id)
{
    printf("start to handle Agent commands\n");

    while (finished() == false)
    {
        auto &skt = sockets_map[conn_id];

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

        // validate command
        if (cmd.src_conn_id != conn_id || (cmd.type != CommandType::CMD_TRANSFER_COMPUTE_BLK && cmd.type != CommandType::CMD_TRANSFER_RELOC_BLK) || cmd.src_node_id == cmd.dst_node_id)
        {
            fprintf(stderr, "invalid command content\n");
            exit(EXIT_FAILURE);
        }

        printf("received block transfer task and block\n");
        cmd.print();
    }
}
