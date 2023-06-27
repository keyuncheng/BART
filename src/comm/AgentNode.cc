#include "AgentNode.hh"

AgentNode::AgentNode(uint16_t _self_conn_id, Config &_config) : Node(_self_conn_id, _config)
{
    // create command distribution queues
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        cmd_dist_queues[conn_id] = new MessageQueue<Command>(MAX_MSG_QUEUE_LEN);
    }

    // create compute task queues
    for (unsigned int cmp_worker_id = 0; cmp_worker_id < config.num_compute_workers; cmp_worker_id++)
    {
        compute_task_queues[cmp_worker_id] = new MessageQueue<ParityComputeTask>(MAX_MSG_QUEUE_LEN);
    }

    // create relocation task queues
    for (unsigned int reloc_worker_id = 0; reloc_worker_id < config.num_reloc_workers; reloc_worker_id++)
    {
        reloc_task_queues[reloc_worker_id] = new MultiWriterQueue<Command>(MAX_MSG_QUEUE_LEN);
    }

    // create block request handler
    block_req_handler = new BlockReqHandler(config, self_conn_id, config.settings.num_nodes); // DEBUG

    // create command distributor
    cmd_distributor = new CmdDist(config, self_conn_id, connectors_map, cmd_dist_queues);

    // create command handler
    cmd_handler = new CmdHandler(config, self_conn_id, sockets_map, &cmd_dist_queues, &compute_task_queues, &reloc_task_queues);

    // create compute workers
    for (unsigned int cmp_worker_id = 0; cmp_worker_id < config.num_compute_workers; cmp_worker_id++)
    {
        compute_workers[cmp_worker_id] = new ComputeWorker(config, cmp_worker_id, self_conn_id, *compute_task_queues[cmp_worker_id], reloc_task_queues);
    }

    // create relocation workers
    for (unsigned int reloc_worker_id = 0; reloc_worker_id < config.num_reloc_workers; reloc_worker_id++)
    {
        reloc_workers[reloc_worker_id] = new RelocWorker(config, reloc_worker_id, self_conn_id, *reloc_task_queues[reloc_worker_id]);
    }
}

AgentNode::~AgentNode()
{
    // delete relocation workers
    for (unsigned int reloc_worker_id = 0; reloc_worker_id < config.num_reloc_workers; reloc_worker_id++)
    {
        delete reloc_workers[reloc_worker_id];
    }

    // delete compute workers
    for (unsigned int cmp_worker_id = 0; cmp_worker_id < config.num_compute_workers; cmp_worker_id++)
    {
        delete compute_workers[cmp_worker_id];
    }

    // delete command handler
    delete cmd_handler;

    // delete command distributor
    delete cmd_distributor;

    // delete block request handler
    delete block_req_handler;

    // delete relocation task queues
    for (unsigned int reloc_worker_id = 0; reloc_worker_id < config.num_reloc_workers; reloc_worker_id++)
    {
        delete reloc_task_queues[reloc_worker_id];
    }

    // delete compute task queues
    for (unsigned int cmp_worker_id = 0; cmp_worker_id < config.num_compute_workers; cmp_worker_id++)
    {
        delete compute_task_queues[cmp_worker_id];
    }

    // delete command distribution queues
    for (auto &item : connectors_map)
    {
        uint16_t conn_id = item.first;
        delete cmd_dist_queues[conn_id];
    }
}

void AgentNode::start()
{
    // start block request handlers
    block_req_handler->start();

    // start reloc workers
    for (unsigned int reloc_worker_id = 0; reloc_worker_id < config.num_reloc_workers; reloc_worker_id++)
    {
        reloc_workers[reloc_worker_id]->start();
    }

    // start compute workers
    for (unsigned int cmp_worker_id = 0; cmp_worker_id < config.num_compute_workers; cmp_worker_id++)
    {
        compute_workers[cmp_worker_id]->start();
    }

    // start cmd_distributor
    cmd_distributor->start();

    // start cmd_handler
    cmd_handler->start();
}

void AgentNode::stop()
{
    // wait reloc_workers finish
    for (unsigned int reloc_worker_id = 0; reloc_worker_id < config.num_reloc_workers; reloc_worker_id++)
    {
        reloc_workers[reloc_worker_id]->wait();
    }

    // wait compute workers finish
    for (unsigned int cmp_worker_id = 0; cmp_worker_id < config.num_compute_workers; cmp_worker_id++)
    {
        compute_workers[cmp_worker_id]->wait();
    }

    // wait cmd_distributor finish
    cmd_distributor->wait();

    // wait cmd_handler finish
    cmd_handler->wait();

    // wait block request handler
    block_req_handler->setFinished();
    block_req_handler->wait();
}