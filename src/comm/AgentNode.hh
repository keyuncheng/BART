#ifndef __AGENT_NODE_HH__
#define __AGENT_NODE_HH__

#include "../include/include.hh"
#include "../util/Config.hh"
#include "../util/MessageQueue.hh"
#include "Node.hh"
#include "CmdDist.hh"
#include "CmdHandler.hh"
#include "ParityComputeTask.hh"
#include "ComputeWorker.hh"
#include "WriteWorker.hh"

class AgentNode : public Node
{
private:
    /* data */
public:
    CmdHandler *cmd_handler;  // handler commands from Controller and Agents
    CmdDist *cmd_distributor; // distribute send block commands
    ComputeWorker *compute_worker;

    // queue for command distribution
    MessageQueue<Command> *cmd_dist_queue;
    MessageQueue<ParityComputeTask> *parity_compute_queue;

    AgentNode(uint16_t _self_conn_id, Config &_config);
    ~AgentNode();

    void start();
    void stop();
};

#endif // __AGENT_NODE_HH__