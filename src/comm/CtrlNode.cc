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

void CtrlNode::read_placement(string placement_file)
{
    StripeGenerator generator;

    stripes.clear();
    generator.loadStripes(config.code, config.settings, placement_file, stripes);

    // TODO: create block name to stripe mapping
}

void CtrlNode::gen_commands()
{
    // solutions and load distribution
    TransSolution trans_solution(config.code, config.settings);

    mt19937 random_generator = Utils::createRandomGenerator();

    if (config.approach == "SM")
    {
        // StripeMerge
        StripeMerge stripe_merge(random_generator);
        StripeBatch stripe_batch(0, config.code, config.settings, random_generator, stripes);
        stripe_merge.genTransSolution(stripe_batch, trans_solution);
        trans_solution.print();
    }
    else if (config.approach == "BT")
    {
        // Balanced Conversion
        BalancedConversion balanced_conversion(random_generator);
        StripeBatch stripe_batch(0, config.code, config.settings, random_generator, stripes);
        balanced_conversion.genTransSolution(stripe_batch, trans_solution);
        trans_solution.print();
    }

    // TODO: parse the tran_solution into commands and add to cmd_distributor
    // read, write, transfer, delete, compute
    // especially the filename, coefficient, ....
}