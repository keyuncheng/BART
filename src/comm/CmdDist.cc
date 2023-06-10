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

    // init counter
    num_finished_connectors = 0;
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
    printf("CmdDist::run start to distribute commands\n");
    while (finished() == false || cmd_dist_queue.IsEmpty() == false)
    {
        Command cmd;
        if (cmd_dist_queue.Pop(cmd) == true)
        {
            cmd.print();

            // add lock
            unique_lock<mutex> lck(*mtxs_map[cmd.dst_conn_id]);

            // send the command
            auto &connector = connectors_map[cmd.dst_conn_id];
            if (connector.write_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            // check if the command is a block transfer command sent from Agent, after which there will be a following block comming transferred in
            if (cmd.src_conn_id != CTRL_NODE_ID && (cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK))
            {
                if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
                {
                    // relocating newly generated parity block
                    if (cmd.post_block_id > config.code.k_f)
                    {
                        // make sure the parity block has been generated from compute worker
                        while (true)
                        {
                            ifstream f(cmd.src_block_path.c_str());
                            if (f.good() == false)
                            {
                                this_thread::sleep_for(chrono::milliseconds(1));
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }

                // read block
                if (BlockIO::readBlock(cmd.src_block_path, block_buffer, config.block_size) != config.block_size)
                {
                    fprintf(stderr, "error reading block: %s\n", cmd.src_block_path.c_str());
                    exit(EXIT_FAILURE);
                }

                // send block
                if (BlockIO::sendBlock(connector, block_buffer, config.block_size) != config.block_size)
                {
                    fprintf(stderr, "error sending block: %s to Node %u\n", cmd.src_block_path.c_str(), cmd.dst_conn_id);
                    exit(EXIT_FAILURE);
                }

                printf("CmdDist::run obtained block transfer task, send block %s\n", cmd.src_block_path.c_str());
                // Utils::printUCharBuffer(block_buffer, 10);
            }

            // stop connection command
            if (cmd.type == CommandType::CMD_STOP)
            {
                connector.close();
                num_finished_connectors++;
                printf("CmdDist::run obtained stop connection command to Node %u, call connector.close()\n", cmd.dst_conn_id);

                if (num_finished_connectors == connectors_map.size())
                {
                    printf("CmdDist::run all Nodes connection stopped, finished\n");
                    setFinished();
                }
            }
        }
    }
}