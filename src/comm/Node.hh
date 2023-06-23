#ifndef __NODE_HH__
#define __NODE_HH__

#include "../include/include.hh"
#include <chrono>
#include <thread>
#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include "../util/Config.hh"
#include "Command.hh"

class Node
{
private:
    /* data */
public:
    uint16_t self_conn_id; // 0 - node_id: Agent;
    Config &config;
    unordered_map<uint16_t, pair<string, unsigned int>> addrs_map;
    unordered_map<uint16_t, sockpp::tcp_connector> connectors_map;
    unordered_map<uint16_t, sockpp::tcp_socket> sockets_map;
    sockpp::tcp_acceptor *acceptor;

    Node(uint16_t _self_conn_id, Config &_config);
    ~Node();

    // sockets connections
    static void connectAllSockets(uint16_t self_conn_id, unordered_map<uint16_t, sockpp::tcp_connector> *connectors_map, unordered_map<uint16_t, pair<string, unsigned int>> *addrs_map);
    static void connectOneSocket(uint16_t self_conn_id, unordered_map<uint16_t, sockpp::tcp_connector> *connectors_map, uint16_t conn_id, string ip, uint16_t port);
    static void handleAckOneSocket(uint16_t self_conn_id, unordered_map<uint16_t, sockpp::tcp_connector> *connectors_map, uint16_t conn_id);
    static void ackConnAllSockets(uint16_t self_conn_id, unordered_map<uint16_t, sockpp::tcp_socket> *sockets_map, sockpp::tcp_acceptor *acceptor);
};

#endif // __NODE_HH__