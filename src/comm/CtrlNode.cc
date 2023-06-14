#include "CtrlNode.hh"

CtrlNode::CtrlNode(uint16_t _self_conn_id, Config &_config) : Node(_self_conn_id, _config)
{
    // create command distribution queues
    // each connector have one distinct queue (self_conn_id don't have it)
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        cmd_dist_queues[conn_id] = new MessageQueue<Command>(MAX_MSG_QUEUE_LEN);
    }

    // create command handler (only handle STOP command from Agents)
    cmd_handler = new CmdHandler(config, self_conn_id, sockets_map, NULL, NULL, NULL, NULL, NULL, 1);

    // create command distributor
    cmd_distributor = new CmdDist(config, connectors_map, cmd_dist_queues, 1);
}

CtrlNode::~CtrlNode()
{
    delete cmd_distributor;
    delete cmd_handler;

    // each connector have one distinct queue
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        delete cmd_dist_queues[conn_id];
    }
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
    trans_solution.print();

    vector<Command> commands;

    // // generate transition commands
    genCommands(stripe_batch, trans_solution, pre_block_mapping, post_block_mapping, commands);
    // genSampleCommands(commands);

    // generate stop commands
    for (auto &item : connectors_map)
    {
        uint16_t dst_conn_id = item.first;

        Command cmd_disconnect;
        cmd_disconnect.buildCommand(CommandType::CMD_STOP, self_conn_id, dst_conn_id, INVALID_STRIPE_ID, INVALID_BLK_ID, INVALID_NODE_ID, INVALID_NODE_ID, string(), string());

        commands.push_back(cmd_disconnect);
    }

    // print commands
    printf("Generated Commands:\n");
    for (auto &command : commands)
    {
        command.print();
    }
    printf("\n\n");

    for (auto &command : commands)
    {
        // command.print();
        cmd_dist_queues[command.dst_conn_id]->Push(command);
    }
}

void CtrlNode::genCommands(StripeBatch &stripe_batch, TransSolution &trans_solution, vector<vector<pair<uint16_t, string>>> &pre_block_mapping, vector<vector<pair<uint16_t, string>>> &post_block_mapping, vector<Command> &commands)
{
    ConvertibleCode &code = config.code;

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
            case TransTaskType::COMPUTE_RE_BLK:
            {
                is_task_parsed = true;
                cmd_type = CommandType::CMD_COMPUTE_RE_BLK;
                break;
            }
            case TransTaskType::COMPUTE_PM_BLK:
            {
                is_task_parsed = true;
                cmd_type = CommandType::CMD_COMPUTE_PM_BLK;
                break;
            }
            case TransTaskType::READ_RE_BLK:
            case TransTaskType::READ_PM_BLK:
            {
                uint16_t parity_compute_node = INVALID_NODE_ID;

                if (task->type == READ_RE_BLK)
                {
                    parity_compute_node = stripe_batch.selected_sgs.at(sg_id).parity_comp_nodes[0];
                }
                else if (task->type == READ_PM_BLK)
                {
                    parity_compute_node = stripe_batch.selected_sgs.at(sg_id).parity_comp_nodes[task->post_block_id - code.k_f];
                }

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
                    exit(EXIT_FAILURE);
                }

                // parse the task
                is_task_parsed = true;
                cmd_type = CommandType::CMD_READ_COMPUTE_BLK;

                // src block path
                src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;

                // dst block path
                if (task->type == TransTaskType::READ_RE_BLK)
                { // need to read the path of all parity blocks, as we only have one parity computation task
                    for (uint8_t parity_id = 0; parity_id < config.code.m_f; parity_id++)
                    { // concatenate the paths
                        if (parity_id > 0)
                        {
                            dst_block_path = dst_block_path + ":";
                        }
                        dst_block_path = dst_block_path + post_block_mapping[sg_id][config.code.k_f + parity_id].second;
                    }
                }
                else if (task->type == TransTaskType::READ_PM_BLK)
                { // destination block: obtained the corresponding parity block only
                    dst_block_path = to_string(task->pre_stripe_id_relative) + string(":") + post_block_mapping[task->post_stripe_id][task->post_block_id].second;
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
                    exit(EXIT_FAILURE);
                }

                // check block location
                uint16_t src_block_node_id = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].first;
                if (task->src_node_id != src_block_node_id)
                {
                    fprintf(stderr, "error: inconsistent block location: %u, %u\n", src_block_node_id, task->src_node_id);
                    exit(EXIT_FAILURE);
                }

                // parse the task
                is_task_parsed = true;
                cmd_type = CommandType::CMD_TRANSFER_COMPUTE_BLK;

                // src block path
                src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;

                // dst block path
                if (task->type == TransTaskType::TRANSFER_COMPUTE_RE_BLK)
                {
                    for (uint8_t parity_id = 0; parity_id < config.code.m_f; parity_id++)
                    {
                        if (parity_id > 0)
                        {
                            dst_block_path = dst_block_path + ":";
                        }
                        dst_block_path = dst_block_path + post_block_mapping[sg_id][config.code.k_f + parity_id].second;
                    }
                }
                else if (task->type == TransTaskType::TRANSFER_COMPUTE_PM_BLK)
                {
                    dst_block_path = to_string(task->pre_stripe_id_relative) + string(":") + post_block_mapping[task->post_stripe_id][task->post_block_id].second;
                }

                break;
            }
            case TransTaskType::TRANSFER_RELOC_BLK:
            {
                // check task src/dst
                if (task->src_node_id == task->dst_node_id)
                {
                    fprintf(stderr, "error: invalid transfer task: (%u -> %u)\n", task->src_node_id, task->dst_node_id);
                    exit(EXIT_FAILURE);
                }

                // parse the task
                is_task_parsed = true;
                cmd_type = CommandType::CMD_TRANSFER_RELOC_BLK;

                if (task->post_block_id < code.k_f)
                { // data block
                    // check block location (for data block only)
                    uint16_t src_block_node_id = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].first;

                    if (task->src_node_id != src_block_node_id)
                    {
                        fprintf(stderr, "error: inconsistent block location: %u, %u\n", src_block_node_id, task->src_node_id);
                        exit(EXIT_FAILURE);
                    }
                    // data block path: obtained from pre_block_mapping
                    src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                }
                else
                { // newly computed parity block path (no need to check the location)
                    // the same path (but on different node)
                    src_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;
                }

                dst_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;

                break;
            }
            case TransTaskType::DELETE_BLK:
            {
                is_task_parsed = true;
                cmd_type = CommandType::CMD_DELETE_BLK;

                if (task->post_block_id < code.k_f)
                {
                    src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                }
                else
                {
                    if (task->pre_stripe_id_global != INVALID_STRIPE_ID_GLOBAL && task->pre_stripe_id_relative != INVALID_STRIPE_ID && task->pre_block_id != INVALID_BLK_ID && task->post_block_id == INVALID_BLK_ID)
                    { // old parity block
                        src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                    }
                    else
                    { // relocated parity block
                        src_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;
                    }
                }

                dst_block_path = ""; // empty string
                break;
            }
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