#ifndef __NODE_HH__
#define __NODE_HH__

#include "../include/include.hh"
#include <chrono>
#include <thread>
#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include "../util/Config.hh"
#include "Command.hh"
#include "CmdHandler.hh"

class Node
{
private:
    /* data */
public:
    uint16_t self_conn_id; // 0 - node_id: Agent;
    Config &config;
    unordered_map<uint16_t, sockpp::tcp_connector> connectors_map;
    sockpp::tcp_acceptor *acceptor;
    CmdHandler *cmd_handler;

    Node(uint16_t _self_conn_id, Config &_config);
    ~Node();

    void start();
    void stop();

    void connect_all();
    void connect_one(uint16_t conn_id, string ip, uint16_t port);
    void ack_one(uint16_t conn_id);
    void disconnect_all();
    void disconnect_one(uint16_t conn_id);
};

#endif // __NODE_HH__