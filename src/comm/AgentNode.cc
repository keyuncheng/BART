#include "AgentNode.hh"

AgentNode::AgentNode(uint16_t _self_conn_id, Config &_config) : Node(_self_conn_id, _config)
{

    // create command handler
    cmd_handler = new CmdHandler(connectors_map, sockets_map, *acceptor, 1);
}

AgentNode::~AgentNode()
{
    delete cmd_handler;
}

void AgentNode::start()
{
    // start cmd_handler
    cmd_handler->start();
}

void AgentNode::stop()
{
    // wait cmd_handler finish
    if (cmd_handler->finished())
    {
        cmd_handler->wait();
    }
}