#include "AgentNode.hh"

AgentNode::AgentNode(uint16_t _self_conn_id, Config &_config) : Node(_self_conn_id, _config)
{
    // create command distribution queues
    // each connector have one distinct queue (self_conn_id don't have it)
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        cmd_dist_queues[conn_id] = new MessageQueue<Command>(MAX_MSG_QUEUE_LEN);
    }

    // create parity compute task queue
    parity_compute_queue = new MultiWriterQueue<ParityComputeTask>(MAX_PARITY_COMPUTE_QUEUE_LEN * config.code.n_f);

    // create parity compute result queue
    parity_comp_result_queue = new MessageQueue<string>(MAX_PARITY_COMPUTE_QUEUE_LEN);

    // memory pool
    memory_pool = new MemoryPool(MAX_MEM_POOL_SIZE * config.code.lambda_i * config.agent_addr_map.size(), config.block_size);

    // create compute worker
    compute_worker = new ComputeWorker(config, *parity_compute_queue, *parity_comp_result_queue, *memory_pool, 1);

    // create command distributor
    cmd_distributor = new CmdDist(config, connectors_map, cmd_dist_queues, 1);

    // create command handler
    cmd_handler = new CmdHandler(config, self_conn_id, sockets_map, &cmd_dist_queues, parity_compute_queue, parity_comp_result_queue, memory_pool, compute_worker, 1);
}

AgentNode::~AgentNode()
{
    delete cmd_handler;
    delete cmd_distributor;
    delete compute_worker;
    delete memory_pool;
    delete parity_comp_result_queue;
    delete parity_compute_queue;

    // each connector have one distinct queue
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        delete cmd_dist_queues[conn_id];
    }
}

void AgentNode::start()
{
    // start cmd_handler
    cmd_handler->start();

    // start cmd_distributor
    cmd_distributor->start();

    // start compute_worker
    compute_worker->start();
}

void AgentNode::stop()
{
    // wait compute_worker finish
    compute_worker->wait();

    // wait cmd_distributor finish
    cmd_distributor->wait();

    // wait cmd_handler finish
    cmd_handler->wait();
}