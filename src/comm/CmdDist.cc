#include "CmdDist.hh"

CmdDist::CmdDist(Config &_config, unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, MessageQueue<Command> *> &_cmd_dist_queues, unsigned int _num_threads) : ThreadPool(_num_threads), config(_config), connectors_map(_connectors_map), cmd_dist_queues(_cmd_dist_queues)
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
    printf("CmdDist::distCmdToController start to distribute commands to Controller\n");

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
            if (cmd.dst_conn_id != dst_conn_id)
            {
                fprintf(stderr, "CmdDist::distCmdToController error: invalid command content: %u, %u\n", cmd.dst_conn_id, dst_conn_id);
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

    printf("CmdDist::distCmdToController finished distribute commands to Controller\n");
}

void CmdDist::distCmdToAgent(uint16_t dst_conn_id)
{
    printf("CmdDist::distCmdToAgent start to distribute commands to Agent %u\n", dst_conn_id);

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
    unsigned char *block_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

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
            if (cmd.dst_conn_id != dst_conn_id)
            {
                fprintf(stderr, "CmdDist::distControllerCmd error: invalid command content\n");
                exit(EXIT_FAILURE);
            }

            cmd.print();

            // send the command
            if (connector.write_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "CmdDist::distCmdToAgent error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            // For block transfer command sent from Agent only
            if (cmd.src_conn_id != CTRL_NODE_ID)
            {
                if (cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
                { // after sending the command to Agent, we send the corresponding block
                    // read block
                    if (BlockIO::readBlock(cmd.src_block_path, block_buffer, config.block_size) != config.block_size)
                    {
                        fprintf(stderr, "CmdDist::distCmdToAgent error reading block: %s\n", cmd.src_block_path.c_str());
                        exit(EXIT_FAILURE);
                    }

                    // send block
                    if (BlockIO::sendBlock(connector, block_buffer, config.block_size) != config.block_size)
                    {
                        fprintf(stderr, "CmdDist::distCmdToAgent error sending block: %s to Node %u\n", cmd.src_block_path.c_str(), cmd.dst_conn_id);
                        exit(EXIT_FAILURE);
                    }

                    printf("CmdDist::distCmdToAgent obtained block transfer task, send block %s\n", cmd.src_block_path.c_str());
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
    free(block_buffer);

    printf("CmdDist::distCmdToAgent finished distribute commands to Agent %u\n", dst_conn_id);
}

// void CmdDist::distControllerCmd()
// {
//     printf("CmdDist::run start to distribute commands\n");
//     while (true)
//     {
//         if (finished() == true && cmd_dist_queue.IsEmpty() == true)
//         {
//             break;
//         }

//         Command cmd;
//         if (cmd_dist_queue.Pop(cmd) == true)
//         {
//             cmd.print();

//             // add lock
//             unique_lock<mutex> lck(*mtxs_map[cmd.dst_conn_id]);

//             // send the command
//             auto &connector = connectors_map[cmd.dst_conn_id];
//             if (connector.write_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
//             {
//                 fprintf(stderr, "error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
//                 exit(EXIT_FAILURE);
//             }

//             //  sent from Agent, after which there will be a following block comming transferred in
//             if (cmd.src_conn_id != CTRL_NODE_ID)
//             { // check commands from Agent's CmdHandler
//                 if (cmd.type == CommandType::CMD_TRANSFER_COMPUTE_BLK || cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
//                 {
//                     // // if it's a newly generated parity block, then we need to check whether it has been computed
//                     // if (cmd.type == CommandType::CMD_TRANSFER_RELOC_BLK)
//                     // {
//                     //     // relocating newly generated parity block
//                     //     if (cmd.post_block_id >= config.code.k_f)
//                     //     {
//                     //         // hack
//                     //         break;

//                     //         while (true)
//                     //         { // make sure the parity block has been generated and stored from compute worker
//                     //             if (compute_worker->isTaskOngoing(EncodeMethod::RE_ENCODE, cmd.post_stripe_id, cmd.post_block_id) == true || compute_worker->isTaskOngoing(EncodeMethod::PARITY_MERGE, cmd.post_stripe_id, cmd.post_block_id) == true)
//                     //             {
//                     //                 this_thread::sleep_for(chrono::milliseconds(10));
//                     //                 continue;
//                     //             }
//                     //             else
//                     //             {
//                     //                 break;
//                     //             }
//                     //         }
//                     //     }
//                     // }

//                     // read block
//                     if (BlockIO::readBlock(cmd.src_block_path, block_buffer, config.block_size) != config.block_size)
//                     {
//                         fprintf(stderr, "error reading block: %s\n", cmd.src_block_path.c_str());
//                         exit(EXIT_FAILURE);
//                     }

//                     // send block
//                     if (BlockIO::sendBlock(connector, block_buffer, config.block_size) != config.block_size)
//                     {
//                         fprintf(stderr, "error sending block: %s to Node %u\n", cmd.src_block_path.c_str(), cmd.dst_conn_id);
//                         exit(EXIT_FAILURE);
//                     }

//                     printf("CmdDist::run obtained block transfer task, send block %s\n", cmd.src_block_path.c_str());
//                     // Utils::printUCharBuffer(block_buffer, 10);
//                 }
//             }

//             if (cmd.type == CommandType::CMD_STOP)
//             { // stop connection command

//                 unique_lock<mutex> lck(finished_mtx);
//                 num_finished_connectors++;
//                 lck.unlock();

//                 printf("CmdDist::run obtained stop connection command to Node %u\n", cmd.dst_conn_id);

//                 if (num_finished_connectors == connectors_map.size())
//                 {
//                     printf("CmdDist::run all Nodes connection stopped, finished\n");
//                     setFinished();
//                 }
//             }
//         }
//     }

//     // close connectors
//     for (auto &item : connectors_map)
//     {
//         item.second.close();
//     }

//     printf("CmdDist::run stop to distribute commands\n");
// }