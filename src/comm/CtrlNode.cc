#include "CtrlNode.hh"

CtrlNode::CtrlNode(uint16_t _self_conn_id, Config &_config) : Node(_self_conn_id, _config)
{
    // create command distributor
    cmd_distributor = new CmdDist(connectors_map, sockets_map, *acceptor, 1);
}

CtrlNode::~CtrlNode()
{
    delete cmd_distributor;
}

void CtrlNode::start()
{
    // start cmd_handler
    cmd_distributor->start();
}

void CtrlNode::stop()
{
    // wait cmd_distributor finish
    if (cmd_distributor->finished())
    {
        cmd_distributor->wait();
    }
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
    gen_commands(trans_solution, pre_block_mapping, post_block_mapping, commands);

    for (auto &command : commands)
    {
        command.print();
    }
}

void CtrlNode::gen_commands(TransSolution &trans_solution, vector<vector<pair<uint16_t, string>>> &pre_block_mapping, vector<vector<pair<uint16_t, string>>> &post_block_mapping, vector<Command> &commands)
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
                src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                dst_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;

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
                src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                dst_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;

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
                // is_task_parsed = true;
                // cmd_type = CommandType::CMD_DELETE_BLK;
                // src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                // dst_block_path = ""; // empty string
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