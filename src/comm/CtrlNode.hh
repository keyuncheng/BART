#ifndef __CTRL_NODE_HH__
#define __CTRL_NODE_HH__

#include "../include/include.hh"
#include "../util/Config.hh"
#include "Node.hh"
#include "CmdDist.hh"
#include "../util/StripeGenerator.hh"
#include "../model/TransSolution.hh"
#include "../model/StripeMerge.hh"
#include "../model/BalancedConversion.hh"

class CtrlNode : public Node
{
private:
    /* data */
public:
    CmdDist *cmd_distributor;

    vector<Stripe> stripes;

    CtrlNode(uint16_t _self_conn_id, Config &_config);
    ~CtrlNode();

    void start();
    void stop();

    void read_placement(string filename);
    void gen_commands();
};

#endif // __CTRL_NODE_HH__
