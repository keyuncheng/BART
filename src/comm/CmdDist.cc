#include "CmdDist.hh"

CmdDist::CmdDist(Config &_config, unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, MessageQueue<Command> &_cmd_dist_queue, unsigned int _num_threads) : ThreadPool(_num_threads), config(_config), connectors_map(_connectors_map), cmd_dist_queue(_cmd_dist_queue)
{
    // init mutex
    for (auto &item : connectors_map)
    {
        mtxs_map[item.first] = new mutex();
    }

    // init block buffer
    block_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));
}

CmdDist::~CmdDist()
{
    free(block_buffer);

    for (auto &item : mtxs_map)
    {
        delete mtxs_map[item.first];
    }
}

void CmdDist::run()
{
    printf("start to distribute command\n");
    while (finished() == false || cmd_dist_queue.IsEmpty() == false)
    {
        Command cmd;
        if (cmd_dist_queue.Pop(cmd) == true)
        {
            unique_lock<mutex> lck(*mtxs_map[cmd.dst_conn_id]); // add lock
            auto &connector = connectors_map[cmd.dst_conn_id];
            // send the command
            if (connector.write_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "error send cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            cmd.print();

            // check if the command is block transfer command sent from Agent
            if ((cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK) && cmd.src_conn_id != CTRL_NODE_ID)
            {
                // read block
                if (BlockIO::readBlock(cmd.src_block_path, block_buffer, config.block_size) != config.block_size)
                {
                    fprintf(stderr, "invalid block: %s\n", cmd.src_block_path.c_str());
                    exit(EXIT_FAILURE);
                }

                // send block
                if (BlockIO::sendBlock(connector, block_buffer, config.block_size) != config.block_size)
                {
                    fprintf(stderr, "error sending block: %s to Node %u\n", cmd.src_block_path.c_str(), cmd.dst_conn_id);
                    exit(EXIT_FAILURE);
                }

                printf("received block transfer task, send block\n");
                // Utils::printUCharBuffer(block_buffer, 10);
            }
        }
    }
}