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
#include "../util/MemoryPool.hh"

class AgentNode : public Node
{
private:
    /* data */
public:
    CmdHandler *cmd_handler;       // handler commands from Controller and Agents
    CmdDist *cmd_distributor;      // distribute send block commands
    ComputeWorker *compute_worker; // compute worker

    // command distribution queues: each retrieves command from CmdHandler and distributes commands to the corresponding CmdDist (CmdHandler -> CmdDist)
    unordered_map<uint16_t, MessageQueue<Command> *> cmd_dist_queues;

    // parity compute task queue: each retrieves parity computation task from Controller and pass to ComputeWorker (CmdHandler -> ComputeWorker)
    unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> pc_task_queues;

    // parity compute result queue: each retrieves parity computation result from ComputeWorker and pass to CmdHandler (ComputeWorker -> CmdHandler)
    unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> pc_reply_queues;

    // parity block relocation task queue (ComputeWorker -> CmdHandler)
    MessageQueue<ParityComputeTask> *parity_reloc_task_queue;

    // memory pool
    MemoryPool *memory_pool;

    AgentNode(uint16_t _self_conn_id, Config &_config);
    ~AgentNode();

    void start();
    void stop();
};

#endif // __AGENT_NODE_HH__