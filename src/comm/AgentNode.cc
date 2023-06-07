#include "AgentNode.hh"

AgentNode::AgentNode(uint16_t _self_conn_id, Config &_config) : Node(_self_conn_id, _config)
{
    // create command distribution queue
    cmd_dist_queue = new MessageQueue<Command>(MAX_MSG_QUEUE_LEN);

    // create command distributor
    cmd_distributor = new CmdDist(config, connectors_map, *cmd_dist_queue, 1);
    // create command handler
    cmd_handler = new CmdHandler(config, sockets_map, *cmd_dist_queue, 1);
}

AgentNode::~AgentNode()
{
    delete cmd_distributor;
    delete cmd_handler;
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
    // wait cmd_distributor finish
    cmd_distributor->wait();

    // wait cmd_handler finish
    cmd_handler->wait();
}