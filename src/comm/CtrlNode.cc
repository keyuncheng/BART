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

    StripeGenerator generator;

    StripeBatch stripe_batch(0, config.code, config.settings, random_generator);
    TransSolution trans_solution(config.code, config.settings);

    // read block mapping file for pre stripe placement
    vector<vector<pair<uint16_t, string>>> pre_stripe_placements;

    // store block mapping file for post stripe placement
    vector<vector<pair<uint16_t, string>>> post_stripe_placements;

    // load stripe placement and block mapping
    generator.loadStripes(config.code, config.settings, config.pre_placement_filename, stripe_batch.pre_stripes);
    generator.loadBlockMapping(config.code, config.settings, config.pre_block_mapping_filename, pre_stripe_placements);

    // generate transition solution
    if (config.approach == "RDRE" || config.approach == "RDPM")
    {
        // StripeMerge
        RandomSolution random_solution(random_generator);
        random_solution.genSolution(stripe_batch, config.approach);
    }
    else if (config.approach == "BWRE" || config.approach == "BWPM")
    {
        // StripeMerge
        StripeMerge stripe_merge(random_generator);
        stripe_merge.genSolution(stripe_batch, config.approach);
    }
    else if (config.approach == "BTRE" || config.approach == "BTPM" || config.approach == "BT")
    {
        // Balanced Conversion
        BalancedConversion balanced_conversion(random_generator);
        balanced_conversion.genSolution(stripe_batch, config.approach);
    }

    // build transition tasks
    trans_solution.buildTransTasks(stripe_batch);

    // store placements for post-transition stripes
    generator.storeStripes(stripe_batch.post_stripes, config.post_placement_filename);
    // TODO: store block mapping for post-transition stripes
    // Question: what's the physical path of the newly generated parity blocks?

    vector<Command> commands;
    gen_commands(trans_solution, pre_stripe_placements, commands);
}

void CtrlNode::gen_commands(TransSolution &trans_solution, vector<vector<pair<uint16_t, string>>> &stripe_placements, vector<Command> &commands)
{

    // TODO: parse the tran_solution into commands and add to cmd_distributor
    // read, write, transfer, delete, compute
    // especially the filename, coefficient, ....

    // currently, only parse compute and transfer commands
    for (auto &item : trans_solution.sg_tasks)
    {
        for (auto &task : item.second)
        {
            Command cmd;
            CommandType cmd_type = CommandType::CMD_UNKNOWN;
            string block_path;
            switch (task->type)
            {
            case TransTaskType::READ_RE_BLK:
            {
                cmd_type = CommandType::CMD_READ_RE_BLK;
                block_path = stripe_placements[task->pre_stripe_id_global][task->pre_block_id].second;
                break;
            }
            case TransTaskType::TRANSFER_COMPUTE_RE_BLK:
            {
                cmd_type = CommandType::CMD_TRANSFER_COMPUTE_RE_BLK;
                block_path = stripe_placements[task->pre_stripe_id_global][task->pre_block_id].second;
                break;
            }
            case TransTaskType::COMPUTE_RE_BLK:
            {
                cmd_type = CommandType::CMD_COMPUTE_RE_BLK;
                break;
            }
            case TransTaskType::READ_PM_BLK:
            {
                cmd_type = CommandType::CMD_READ_PM_BLK;
                block_path = stripe_placements[task->pre_stripe_id_global][task->pre_block_id].second;
                break;
            }
            case TransTaskType::TRANSFER_COMPUTE_PM_BLK:
            {
                cmd_type = CommandType::CMD_TRANSFER_COMPUTE_PM_BLK;
                block_path = stripe_placements[task->pre_stripe_id_global][task->pre_block_id].second;
                break;
            }
            case TransTaskType::COMPUTE_PM_BLK:
            {
                cmd_type = CommandType::CMD_COMPUTE_PM_BLK;
                break;
            }
            case TransTaskType::READ_RELOC_BLK:
            {
                cmd_type = CommandType::CMD_READ_RELOC_BLK;
                if (task->post_block_id < trans_solution.code.k_f)
                { // data block relocation
                    block_path = stripe_placements[task->pre_stripe_id_global][task->pre_block_id].second;
                }
                else
                {                             // parity block relocation
                    block_path = "UNDEFINED"; // TODO: figure out the name
                }
                break;
            }
            case TransTaskType::TRANSFER_RELOC_BLK:
            {
                cmd_type = CommandType::CMD_TRANSFER_RELOC_BLK;
                break;
            }
            case TransTaskType::WRITE_BLK:
            {
                cmd_type = CommandType::CMD_WRITE_BLK;
                break;
            }
            case TransTaskType::DELETE_BLK:
            {
                cmd_type = CommandType::CMD_DELETE_BLK;
                break;
            }
            case TransTaskType::UNKNOWN:
            {
                cmd_type = CommandType::CMD_UNKNOWN;
                break;
            }
            }

            // for some tasks, there is no need to generate the commands
            if (cmd_type == CMD_WRITE_BLK || cmd_type == CMD_DELETE_BLK)
            { // no need to generate write and delete for now
                continue;
            }
            else if (cmd_type == CommandType::CMD_READ_RE_BLK || cmd_type == CommandType::CMD_READ_PM_BLK || cmd_type == CommandType::CMD_READ_RELOC_BLK)
            { // for those with transfer task, no need to generate read task
                if (task->src_node_id != task->dst_node_id)
                {
                    continue;
                }
            }

            cmd.buildCommand(cmd_type, self_conn_id, task->src_node_id, task->post_stripe_id, task->post_block_id, task->pre_stripe_id_global, task->pre_stripe_id_relative, task->pre_block_id, task->src_node_id, task->dst_node_id, block_path);

            commands.push_back(cmd);
        }
    }
}