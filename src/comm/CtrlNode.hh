#ifndef __CTRL_NODE_HH__
#define __CTRL_NODE_HH__

#include "../include/include.hh"
#include "../util/Config.hh"
#include "Node.hh"
#include "CmdDist.hh"
#include "../util/StripeGenerator.hh"
#include "../model/TransSolution.hh"
#include "../model/RandomSolution.hh"
#include "../model/StripeMerge.hh"
#include "../util/MessageQueue.hh"
#include "../model/BalancedConversion.hh"

class CtrlNode : public Node
{
private:
    /* data */
public:
    CmdDist *cmd_distributor;

    // queue for command distribution
    MessageQueue<Command> *cmd_dist_queue;

    CtrlNode(uint16_t _self_conn_id, Config &_config);
    ~CtrlNode();

    void start();
    void stop();

    void genTransSolution();
    void genCommands(TransSolution &trans_solution, vector<vector<pair<uint16_t, string>>> &pre_block_mapping, vector<vector<pair<uint16_t, string>>> &post_block_mapping, vector<Command> &commands);

    void genSampleCommands(vector<Command> &commands);
};

#endif // __CTRL_NODE_HH__
