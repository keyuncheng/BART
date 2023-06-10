#include "AgentNode.hh"

AgentNode::AgentNode(uint16_t _self_conn_id, Config &_config) : Node(_self_conn_id, _config)
{
    // create command distribution queue
    cmd_dist_queue = new MessageQueue<Command>(MAX_MSG_QUEUE_LEN);

    // create parity compute task queue
    parity_compute_queue = new MessageQueue<ParityComputeTask>(MAX_MSG_QUEUE_LEN);

    // memory pool
    memory_pool = new MemoryPool(MAX_MEM_POOL_SIZE, config.block_size);

    // create compute worker
    compute_worker = new ComputeWorker(config, *parity_compute_queue, *memory_pool);

    // create command distributor
    cmd_distributor = new CmdDist(config, connectors_map, *cmd_dist_queue, 1);
    // create command handler
    cmd_handler = new CmdHandler(config, sockets_map, cmd_dist_queue, parity_compute_queue, memory_pool, 1);
}

AgentNode::~AgentNode()
{
    delete cmd_handler;
    delete cmd_distributor;
    delete compute_worker;
    delete memory_pool;
    delete parity_compute_queue;
    delete cmd_dist_queue;
}

void AgentNode::start()
{
    // start cmd_handler
    cmd_handler->start();

    // start cmd_distributor
    cmd_distributor->start();
}

void AgentNode::stop()
{
    // wait cmd_handler finish
    cmd_handler->wait_for_controller();

    // send disconnect signal
    disconnectAll();

    cmd_handler->wait_for_agents();

    // set finished
    cmd_distributor->setFinished();

    // wait cmd_distributor finish
    cmd_distributor->wait();
}