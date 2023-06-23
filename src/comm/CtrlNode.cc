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
    cmd_distributor = new CmdDist(config, self_conn_id, connectors_map, cmd_dist_queues, NULL, 1);
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
        cmd_disconnect.buildCommand(CommandType::CMD_STOP, self_conn_id, dst_conn_id);

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
            EncodeMethod enc_method = EncodeMethod::UNKNOWN_METHOD;
            uint8_t num_src_blocks = 0;
            vector<uint16_t> src_block_nodes;
            uint8_t num_parity_reloc_nodes = 0;
            vector<uint16_t> parity_reloc_nodes;

            switch (task->type)
            {
            case TransTaskType::COMPUTE_RE_BLK:
            case TransTaskType::COMPUTE_PM_BLK:
            {
                // parse the task
                is_task_parsed = true;

                // command type
                if (task->type == TransTaskType::COMPUTE_RE_BLK)
                {
                    cmd_type = CommandType::CMD_COMPUTE_RE_BLK;
                }
                else if (task->type == TransTaskType::COMPUTE_PM_BLK)
                {
                    cmd_type = CommandType::CMD_COMPUTE_PM_BLK;
                }

                // uint16_t parity_compute_node = INVALID_NODE_ID;

                bool is_re = task->type == TransTaskType::COMPUTE_RE_BLK;
                if (is_re == true)
                {
                    // encode method;
                    enc_method = EncodeMethod::RE_ENCODE;

                    // parity source nodes
                    num_src_blocks = code.k_f;
                    src_block_nodes.resize(num_src_blocks);
                    for (uint8_t final_data_block_id = 0; final_data_block_id < num_src_blocks; final_data_block_id++)
                    {
                        uint8_t pre_stripe_id = final_data_block_id / code.k_i;
                        uint8_t pre_block_id = final_data_block_id % code.k_i;
                        uint32_t pre_stripe_id_global = stripe_batch.selected_sgs.at(sg_id).pre_stripes[pre_stripe_id]->id;

                        src_block_nodes[final_data_block_id] = pre_block_mapping[pre_stripe_id_global][pre_block_id].first;
                    }

                    // // parity compute node
                    // parity_compute_node = stripe_batch.selected_sgs.at(sg_id).parity_comp_nodes[0];

                    // relocation nodes
                    // for re-encoding, set the relocation nodes for all parity blocks
                    num_parity_reloc_nodes = code.m_f;
                    parity_reloc_nodes.resize(num_parity_reloc_nodes);
                    for (uint8_t parity_id = 0; parity_id < num_parity_reloc_nodes; parity_id++)
                    {
                        parity_reloc_nodes[parity_id] = post_block_mapping[sg_id][config.code.k_f + parity_id].first;
                    }

                    // src paths (concatenate code.k_f paths for all data blocks)
                    string delimiter_char = ":";
                    for (uint8_t final_data_block_id = 0; final_data_block_id < num_src_blocks; final_data_block_id++)
                    {
                        if (final_data_block_id > 0)
                        {
                            src_block_path = src_block_path + delimiter_char;
                        }

                        uint8_t pre_stripe_id = final_data_block_id / code.k_i;
                        uint8_t pre_block_id = final_data_block_id % code.k_i;
                        uint32_t pre_stripe_id_global = stripe_batch.selected_sgs.at(sg_id).pre_stripes[pre_stripe_id]->id;
                        src_block_path = src_block_path + pre_block_mapping[pre_stripe_id_global][pre_block_id].second;
                    }

                    // dst paths (concatenate code.m_f paths for all parity blocks)
                    delimiter_char = ":";
                    for (uint8_t parity_id = 0; parity_id < config.code.m_f; parity_id++)
                    {
                        if (parity_id > 0)
                        {
                            dst_block_path = dst_block_path + delimiter_char;
                        }
                        dst_block_path = dst_block_path + post_block_mapping[sg_id][config.code.k_f + parity_id].second;
                    }
                }

                bool is_pm = (task->type == TransTaskType::COMPUTE_PM_BLK);
                if (is_pm == true)
                {
                    // encode method;
                    enc_method = EncodeMethod::PARITY_MERGE;

                    // parity source nodes
                    num_src_blocks = code.lambda_i;
                    src_block_nodes.resize(num_src_blocks);
                    for (uint8_t pre_stripe_id = 0; pre_stripe_id < num_src_blocks; pre_stripe_id++)
                    {
                        uint32_t pre_stripe_id_global = stripe_batch.selected_sgs.at(sg_id).pre_stripes[pre_stripe_id]->id;
                        src_block_nodes[pre_stripe_id] = pre_block_mapping[pre_stripe_id_global][task->post_block_id - code.k_f + code.k_i].first;
                    }

                    // // parity compute node
                    // parity_compute_node = stripe_batch.selected_sgs.at(sg_id).parity_comp_nodes[task->post_block_id - code.k_f];

                    // relocation nodes
                    // for parity merging: set the relocation node for the corresponding parity block
                    num_parity_reloc_nodes = 1;
                    parity_reloc_nodes.resize(num_parity_reloc_nodes);
                    parity_reloc_nodes[0] = post_block_mapping[sg_id][task->post_block_id].first;

                    // src paths (size: code.lambda_i, concatenating the corresponding parity block paths)
                    string delimiter_char = ":";
                    for (uint8_t pre_stripe_id = 0; pre_stripe_id < config.code.lambda_i; pre_stripe_id++)
                    {
                        if (pre_stripe_id > 0)
                        {
                            src_block_path = src_block_path + delimiter_char;
                        }

                        // pre_stripe_id_global
                        uint32_t pre_stripe_id_global = stripe_batch.selected_sgs.at(sg_id).pre_stripes[pre_stripe_id]->id;
                        src_block_path = src_block_path + pre_block_mapping[pre_stripe_id_global][task->post_block_id - code.k_f + code.k_i].second;
                    }

                    // dst paths (size: 1, corresponding path for the parity block)
                    dst_block_path = post_block_mapping[sg_id][task->post_block_id].second;
                }

                break;
            }
            case TransTaskType::TRANSFER_RELOC_BLK:
            {
                // parse the task
                is_task_parsed = true;
                cmd_type = CommandType::CMD_TRANSFER_RELOC_BLK;

                // only parse for data block relocation; the parity block relocation will be generated by ComputeWorker
                if (task->post_block_id >= code.k_f)
                {
                    is_task_parsed = false;
                    break;
                }

                // data block path: obtained from pre_block_mapping
                src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;

                dst_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;

                break;
            }
            case TransTaskType::DELETE_BLK:
            {
                // DEBUG: set to false
                is_task_parsed = false;

                cmd_type = CommandType::CMD_DELETE_BLK;

                if (task->post_block_id < code.k_f)
                {
                    src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                }
                else
                {
                    if (task->pre_stripe_id_global != INVALID_STRIPE_ID_GLOBAL && task->pre_stripe_id_relative != INVALID_STRIPE_ID && task->pre_block_id != INVALID_BLK_ID && task->post_block_id == INVALID_BLK_ID)
                    { // pre-stripe parity block
                        src_block_path = pre_block_mapping[task->pre_stripe_id_global][task->pre_block_id].second;
                    }
                    else
                    { // post-stripe parity block (before relocation)
                        src_block_path = post_block_mapping[task->post_stripe_id][task->post_block_id].second;
                    }
                }

                dst_block_path = ""; // empty string
                break;
            }
            case TransTaskType::READ_RE_BLK:
            case TransTaskType::READ_PM_BLK:
            case TransTaskType::TRANSFER_COMPUTE_RE_BLK:
            case TransTaskType::TRANSFER_COMPUTE_PM_BLK:
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
                    src_block_path, dst_block_path,
                    enc_method,
                    num_src_blocks, src_block_nodes,
                    num_parity_reloc_nodes, parity_reloc_nodes);
                commands.push_back(cmd);
            }
        }
    }

    printf("generated %lu commands\n", commands.size());
}