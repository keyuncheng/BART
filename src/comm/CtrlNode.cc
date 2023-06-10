#include "CtrlNode.hh"

CtrlNode::CtrlNode(uint16_t _self_conn_id, Config &_config) : Node(_self_conn_id, _config)
{
    // create command distribution queue
    cmd_dist_queue = new MessageQueue<Command>(MAX_MSG_QUEUE_LEN);

    // create command handler (only handle STOP command from Agents)
    cmd_handler = new CmdHandler(config, sockets_map, NULL, NULL, NULL, 1);

    // create command distributor
    cmd_distributor = new CmdDist(config, connectors_map, *cmd_dist_queue, 1);
}

CtrlNode::~CtrlNode()
{
    delete cmd_distributor;
    delete cmd_handler;
    delete cmd_dist_queue;
}

void CtrlNode::start()
{
    // start cmd distributor
    cmd_distributor->start();

    // start cmd_handler
    cmd_handler->start();
}

void CtrlNode::stop()
{
    // wait cmd_distributor finish
    cmd_distributor->wait();

    // wait cmd_handler finish
    cmd_handler->wait();
}

void CtrlNode::genTransSolution()
{
    // random generator
    mt19937 random_generator = Utils::createRandomGenerator();

    ConvertibleCode &code = config.code;
    ClusterSettings &settings = config.settings;

    StripeGenerator generator;

    StripeBatch stripe_batch(0, config.code, config.settings, random_generator);
    TransSolution trans_solution(config.code, config.settings);

    // block mapping file for pre-stripes
    vector<vector<pair<uint16_t, string>>> pre_block_mapping;
    // block mapping file for post-stripes
    vector<vector<pair<uint16_t, string>>> post_block_mapping;

    // load pre-stripe placement and block mapping
    generator.loadStripes(code.n_i, config.pre_placement_filename, stripe_batch.pre_stripes);
    generator.loadBlockMapping(code.n_i, settings.num_stripes, config.pre_block_mapping_filename, pre_block_mapping);

    // load post-stripe placement and block mapping
    generator.loadStripes(code.n_f, config.post_placement_filename, stripe_batch.post_stripes);
    generator.loadBlockMapping(code.n_f, settings.num_stripes / code.lambda_i, config.post_block_mapping_filename, post_block_mapping);

    // load stripe group metadata
    stripe_batch.loadSGMetadata(config.sg_meta_filename);
    // stripe_batch.print();

    // build transition tasks
    trans_solution.buildTransTasks(stripe_batch);
    // trans_solution.print();

    vector<Command> commands;
    // genCommands(trans_solution, pre_block_mapping, post_block_mapping, commands);
    genSampleCommands(commands);

    for (auto &command : commands)
    {
        // command.print();
        cmd_dist_queue->Push(command);
    }

    // send disconnect commands
    for (auto &item : connectors_map)
    {
        uint16_t dst_conn_id = item.first;

        Command cmd_disconnect;
        cmd_disconnect.buildCommand(CommandType::CMD_STOP, self_conn_id, dst_conn_id, INVALID_STRIPE_ID, INVALID_BLK_ID, INVALID_NODE_ID, INVALID_NODE_ID, string(), string());

        cmd_dist_queue->Push(cmd_disconnect);
    }
}

void CtrlNode::genCommands(TransSolution &trans_solution, vector<vector<pair<uint16_t, string>>> &pre_block_mapping, vector<vector<pair<uint16_t, string>>> &post_block_mapping, vector<Command> &commands)
{
    for (auto &item : trans_solution.sg_tasks)
    {
        uint32_t sg_id = item.first;
        // printf("parsing transition tasks for stripe group %u\n", sg_id);
        for (auto &task : item.second)
        {
            Command cmd;

            bool is_task_parsed = false;
            CommandType cmd_type = CommandType::CMD_UNKNOWN;
            string src_block_path;
            string dst_block_path;

            switch (task->type)
            {
            case TransTaskType::READ_RE_BLK:
            case TransTaskType::READ_PM_BLK:
            {

                uint16_t parity_compute_node = post_block_mapping[sg_id][task->post_block_id].first;

                // only parse for local read tasks
                if (task->src_node_id != parity_compute_node)
                {
                    // printf("skip read task at node %u for parity computation at node %u\n", task->src_node_id, task->dst_node_id);
                    continue;
                }

                // check block location
                uint16_t src_block_node_id = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].first;
                if (task->src_node_id != src_block_node_id)
                {
                    fprintf(stderr, "error: inconsistent block location: %u, %u\n", src_block_node_id, task->src_node_id);
                    return;
                }

                // parse the task
                is_task_parsed = true;
                cmd_type = CommandType::CMD_LOCAL_COMPUTE_BLK;

                // src block path
                src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;

                // dst block path
                if (task->type == TransTaskType::READ_RE_BLK)
                {
                    for (uint8_t parity_id = 0; parity_id < config.code.m_f; parity_id++)
                    {
                        if (parity_id > 0)
                        {
                            dst_block_path = dst_block_path + ":";
                        }
                        dst_block_path = dst_block_path + post_block_mapping[task->post_block_id][config.code.k_f + parity_id].second;
                    }
                }
                else if (task->type == TransTaskType::READ_PM_BLK)
                {
                    dst_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;
                }

                break;
            }
            case TransTaskType::TRANSFER_COMPUTE_RE_BLK:
            case TransTaskType::TRANSFER_COMPUTE_PM_BLK:
            {
                // check task src/dst
                if (task->src_node_id == task->dst_node_id)
                {
                    fprintf(stderr, "error: invalid transfer task: (%u -> %u)\n", task->src_node_id, task->dst_node_id);
                    return;
                }

                // check block location
                uint16_t src_block_node_id = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].first;
                if (task->src_node_id != src_block_node_id)
                {
                    fprintf(stderr, "error: inconsistent block location: %u, %u\n", src_block_node_id, task->src_node_id);
                    return;
                }

                // parse the task
                is_task_parsed = true;
                cmd_type = CommandType::CMD_TRANSFER_COMPUTE_BLK;

                // src block path
                src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;

                // dst block path
                if (task->type == TransTaskType::READ_RE_BLK)
                {
                    for (uint8_t parity_id = 0; parity_id < config.code.m_f; parity_id++)
                    {
                        if (parity_id > 0)
                        {
                            dst_block_path = dst_block_path + ":";
                        }
                        dst_block_path = dst_block_path + post_block_mapping[task->post_block_id][config.code.k_f + parity_id].second;
                    }
                }
                else if (task->type == TransTaskType::READ_PM_BLK)
                {
                    dst_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;
                }

                break;
            }
            case TransTaskType::TRANSFER_RELOC_BLK:
            {
                // check task src/dst
                if (task->src_node_id == task->dst_node_id)
                {
                    fprintf(stderr, "error: invalid transfer task: (%u -> %u)\n", task->src_node_id, task->dst_node_id);
                    return;
                }

                // check block location
                uint16_t src_block_node_id = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].first;
                if (task->src_node_id != src_block_node_id)
                {
                    fprintf(stderr, "error: inconsistent block location: %u, %u\n", src_block_node_id, task->src_node_id);
                    return;
                }

                // parse the task
                is_task_parsed = true;
                cmd_type = CommandType::CMD_TRANSFER_RELOC_BLK;
                src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                dst_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;

                break;
            }
            case TransTaskType::DELETE_BLK:
            {
                is_task_parsed = true;
                cmd_type = CommandType::CMD_DELETE_BLK;
                src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                dst_block_path = ""; // empty string
                break;
            }
            case TransTaskType::COMPUTE_RE_BLK:
            case TransTaskType::COMPUTE_PM_BLK:
            case TransTaskType::READ_RELOC_BLK:
            case TransTaskType::WRITE_BLK:
            case TransTaskType::UNKNOWN:
            {
                break;
            }
            }

            if (is_task_parsed == true)
            {
                cmd.buildCommand(
                    cmd_type,
                    self_conn_id,
                    task->src_node_id,
                    task->post_stripe_id,
                    task->post_block_id,
                    task->src_node_id,
                    task->dst_node_id,
                    src_block_path, dst_block_path);
                commands.push_back(cmd);
            }
        }
    }

    printf("generated %lu commands\n", commands.size());
}

void CtrlNode::genSampleCommands(vector<Command> &commands)
{
    // Command cmd_1;
    // cmd_1.buildCommand(
    //     CommandType::CMD_LOCAL_COMPUTE_BLK,
    //     self_conn_id,
    //     0, // cmd to node 0
    //     1, // stripe 1
    //     0, // block 0
    //     0, // from node 0
    //     0, // to node 0
    //     "/home/kycheng/Documents/projects/redundancy-transition/balancedconversion/BalancedConversion/data/64M_1", "/home/kycheng/Documents/projects/redundancy-transition/balancedconversion/BalancedConversion/data/64M_1_solution");
    // commands.push_back(cmd_1);

    // Command cmd_2;
    // cmd_2.buildCommand(
    //     CommandType::CMD_TRANSFER_COMPUTE_BLK,
    //     self_conn_id,
    //     0, // cmd to node 0
    //     1, // stripe 1
    //     1, // block 1
    //     0, // from node 0
    //     1, // to node 1
    //     "/home/kycheng/Documents/projects/redundancy-transition/balancedconversion/BalancedConversion/data/64M_1", "/home/kycheng/Documents/projects/redundancy-transition/balancedconversion/BalancedConversion/data/64M_1_write");
    // commands.push_back(cmd_2);

    // Command cmd_3;
    // cmd_3.buildCommand(
    //     CommandType::CMD_TRANSFER_RELOC_BLK,
    //     self_conn_id,
    //     1, // cmd to node 1
    //     1, // stripe 1
    //     2, // block 2
    //     1, // from node 1
    //     0, // to node 0
    //     "/home/kycheng/Documents/projects/redundancy-transition/balancedconversion/BalancedConversion/data/64M_2", "/home/kycheng/Documents/projects/redundancy-transition/balancedconversion/BalancedConversion/data/64M_2_write");
    // commands.push_back(cmd_3);

    // Command cmd_4;
    // cmd_4.buildCommand(
    //     CommandType::CMD_DELETE_BLK,
    //     self_conn_id,
    //     1, // cmd to node 1
    //     1, // stripe 1
    //     3, // block 3
    //     1, // from node 1
    //     1, // to node 1
    //     "sample_src_block_path", "sample_dst_block_path");
    // commands.push_back(cmd_4);
}