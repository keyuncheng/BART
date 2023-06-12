#include "CmdDist.hh"

CmdDist::CmdDist(Config &_config, unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, MessageQueue<Command> &_cmd_dist_queue, ComputeWorker *_compute_worker, unsigned int _num_threads) : ThreadPool(_num_threads), config(_config), connectors_map(_connectors_map), cmd_dist_queue(_cmd_dist_queue), compute_worker(_compute_worker)
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
    while (true)
    {
        if (finished() == true && cmd_dist_queue.IsEmpty() == true)
        {
            break;
        }

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

            //  sent from Agent, after which there will be a following block comming transferred in
            if (cmd.src_conn_id != CTRL_NODE_ID)
            { // check commands from Agent's CmdHandler
                if (cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
                {
                    // if it's a newly generated parity block, then we need to check whether it has been computed
                    if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
                    {
                        // relocating newly generated parity block
                        if (cmd.post_block_id >= config.code.k_f)
                        {
                            // hack
                            break;

                            while (true)
                            { // make sure the parity block has been generated and stored from compute worker
                                if (compute_worker->isTaskOngoing(EncodeMethod::RE_ENCODE, cmd.post_stripe_id, cmd.post_block_id) == true || compute_worker->isTaskOngoing(EncodeMethod::PARITY_MERGE, cmd.post_stripe_id, cmd.post_block_id) == true)
                                {
                                    this_thread::sleep_for(chrono::milliseconds(10));
                                    continue;
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
            }

            if (cmd.type == CommandType::CMD_STOP)
            { // stop connection command
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