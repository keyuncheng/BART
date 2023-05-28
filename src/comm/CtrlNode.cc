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

    // read block mapping file
    vector<vector<pair<uint16_t, string>>> stripe_placements;

    // load stripe placement and block mapping
    generator.loadStripes(config.code, config.settings, config.placement_filename, stripe_batch.pre_stripes);
    generator.loadBlockMapping(config.code, config.settings, config.block_mapping_filename, stripe_placements);

    // generate transition solution
    if (config.approach == "SM")
    {
        // StripeMerge
        StripeMerge stripe_merge(random_generator);
        stripe_merge.genSolution(stripe_batch);
    }
    else if (config.approach == "BT")
    {
        // Balanced Conversion
        BalancedConversion balanced_conversion(random_generator);
        balanced_conversion.genSolution(stripe_batch);
    }

    // build transition tasks
    trans_solution.buildTransTasks(stripe_batch);

    vector<Command> commands;
    gen_commands(trans_solution, stripe_placements, commands);
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
            // if (task->type == TransTaskType::TRANSFER_COMPUTE_BLK)
            // {
            //     cmd_type = CommandType::CMD_TRANSFER_COMPUTE_BLK;
            // }
            // else if (task->type == TransTaskType::TRANSFER_RELOC_BLK)
            // {
            //     cmd_type = CommandType::CMD_TRANSFER_RELOC_BLK;
            // }
            // else if (task->type == TransTaskType::COMPUTE_BLK)
            // {
            //     cmd_type = CommandType::CMD_COMPUTE_BLOCK;
            // }

            // cmd.buildCommand(cmd_type, self_conn_id, task->node_id, task->sg_id, task->stripe_id_global, task->stripe_id, task->block_id, task->node_id, task->dst_node_id, )
        }
    }
}