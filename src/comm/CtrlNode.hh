#ifndef __CTRL_NODE_HH__
#define __CTRL_NODE_HH__

#include "../include/include.hh"
#include "../util/Config.hh"
#include "Node.hh"
#include "CmdHandler.hh"
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
    CmdHandler *cmd_handler; // handler commands from Agents
    CmdDist *cmd_distributor;

    // command distribution queue (each is a single reader single writer queue)
    unordered_map<uint16_t, MessageQueue<Command> *> cmd_dist_queues;

    CtrlNode(uint16_t _self_conn_id, Config &_config);
    ~CtrlNode();

    void start();
    void stop();

    void genTransSolution();
    void genCommands(StripeBatch &stripe_batch, TransSolution &trans_solution, vector<vector<pair<uint16_t, string>>> &pre_block_mapping, vector<vector<pair<uint16_t, string>>> &post_block_mapping, vector<Command> &commands);

    void genSampleCommands(vector<Command> &commands);
};

#endif // __CTRL_NODE_HH__
