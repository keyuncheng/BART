#include "AgentNode.hh"

AgentNode::AgentNode(uint16_t _self_conn_id, Config &_config) : Node(_self_conn_id, _config)
{
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;

        // create message queues
        cmd_dist_queues[conn_id] = new MessageQueue<Command>(MAX_MSG_QUEUE_LEN);
    }

    // create parity compute queues
    pc_task_queues[CTRL_NODE_ID] = new MessageQueue<ParityComputeTask>(MAX_MSG_QUEUE_LEN);

    pc_reply_queues[CTRL_NODE_ID] = new MessageQueue<ParityComputeTask>(MAX_MSG_QUEUE_LEN);

    for (uint16_t conn_id = 0; conn_id < config.settings.num_nodes; conn_id++)
    {
        pc_task_queues[conn_id] = new MessageQueue<ParityComputeTask>(MAX_MSG_QUEUE_LEN);

        pc_reply_queues[conn_id] = new MessageQueue<ParityComputeTask>(MAX_MSG_QUEUE_LEN);
    }

    parity_reloc_task_queue = new MessageQueue<ParityComputeTask>(MAX_MSG_QUEUE_LEN);

    // memory pool
    memory_pool = new MemoryPool(MAX_MEM_POOL_SIZE * config.code.lambda_i * config.agent_addr_map.size(), config.block_size);

    // create command distributor, only sending cmd
    cmd_distributor = new CmdDist(config, self_conn_id, connectors_map, cmd_dist_queues, memory_pool, 1);

    // create command handler, receiving cmd and forwarding block
    cmd_handler = new CmdHandler(config, self_conn_id, sockets_map, blk_connectors_map, &cmd_dist_queues, &pc_task_queues, &pc_reply_queues, parity_reloc_task_queue, memory_pool, 1);

    // create compute worker
    compute_worker = new ComputeWorker(config, self_conn_id, sockets_map, pc_task_queues, pc_reply_queues, cmd_dist_queues, *parity_reloc_task_queue, *memory_pool, 1);
}

AgentNode::~AgentNode()
{
    delete cmd_handler;
    delete cmd_distributor;
    delete compute_worker;
    delete memory_pool;

    delete parity_reloc_task_queue;

    for (uint16_t conn_id = 0; conn_id < config.settings.num_nodes; conn_id++)
    {
        delete pc_reply_queues[conn_id];
        delete pc_task_queues[conn_id];
    }

    delete pc_reply_queues[CTRL_NODE_ID];
    delete pc_task_queues[CTRL_NODE_ID];

    // each connector have one distinct queue
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        delete cmd_dist_queues[conn_id];
    }
}

void AgentNode::start()
{
    // start compute_worker
    compute_worker->start();

    // start cmd_distributor
    cmd_distributor->start();

    // start cmd_handler
    cmd_handler->start();
}

void AgentNode::stop()
{
    // wait compute_worker finish
    compute_worker->wait();

    // wait cmd_handler finish
    cmd_handler->wait();

    // wait cmd_distributor finish
    cmd_distributor->wait();
}