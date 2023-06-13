#ifndef __AGENT_NODE_HH__
#define __AGENT_NODE_HH__

#include "../include/include.hh"
#include "../util/Config.hh"
#include "../util/MessageQueue.hh"
#include "../util/MultiWriterQueue.h"
#include "Node.hh"
#include "CmdDist.hh"
#include "CmdHandler.hh"
#include "ParityComputeTask.hh"
#include "ComputeWorker.hh"
#include "WriteWorker.hh"
#include "../util/MemoryPool.hh"

class AgentNode : public Node
{
private:
    /* data */
public:
    CmdHandler *cmd_handler;       // handler commands from Controller and Agents
    CmdDist *cmd_distributor;      // distribute send block commands
    ComputeWorker *compute_worker; // compute worker

    // command distribution queue (each is a single reader single writer queue)
    unordered_map<uint16_t, MessageQueue<Command> *> cmd_dist_queues;

    // parity compute queue (a multiple writer single reader queue)
    MultiWriterQueue<ParityComputeTask> *parity_compute_queue;

    // memory pool
    MemoryPool *memory_pool;

    AgentNode(uint16_t _self_conn_id, Config &_config);
    ~AgentNode();

    void start();
    void stop();
};

#endif // __AGENT_NODE_HH__