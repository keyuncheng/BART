#ifndef __AGENT_NODE_HH__
#define __AGENT_NODE_HH__

#include "../include/include.hh"
#include "../util/Config.hh"
#include "../util/MessageQueue.hh"
#include "../util/MultiWriterQueue.h"
#include "Node.hh"
#include "ParityComputeTask.hh"
#include "CmdHandler.hh"
#include "CmdDist.hh"
#include "ComputeWorker.hh"
#include "RelocWorker.hh"

class AgentNode : public Node
{
private:
    /* data */
public:
    // command distribution queues: each retrieves command from CmdHandler and distributes commands to the corresponding CmdDist (CmdHandler<conn_id> -> CmdDist<conn_id>)
    unordered_map<uint16_t, MessageQueue<Command> *> cmd_dist_queues;

    // compute task queues: each retrieves computation task from Controller, and pass to a ComputeWorker (CmdHandler -> ComputeWorker<worker_id>)
    unordered_map<unsigned int, MessageQueue<ParityComputeTask> *> compute_task_queues;

    // block relocation task queue: each retrieves block relocation task (from both CmdHandler and ComputeWorker), and pass to a RelocWorker (ComputerWorker / CmdHandler -> RelocWorker<worker_id>)
    unordered_map<unsigned int, MultiWriterQueue<Command> *> reloc_task_queues;

    // distribute commands
    CmdDist *cmd_distributor;

    // handler commands
    CmdHandler *cmd_handler;

    // compute workers <worker_id, worker>
    unordered_map<unsigned int, ComputeWorker *> compute_workers;

    // relocation workers <worker_id, worker>
    unordered_map<unsigned int, RelocWorker *> reloc_workers;

    AgentNode(uint16_t _self_conn_id, Config &_config);
    ~AgentNode();

    void start();
    void stop();
};

#endif // __AGENT_NODE_HH__