#ifndef __CMD_DIST_HH__
#define __CMD_DIST_HH__

#include <mutex>
#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include "../include/include.hh"
#include "../util/Utils.hh"
#include "../util/MessageQueue.hh"
#include "Command.hh"
#include "../util/ThreadPool.hh"

class CmdDist : public ThreadPool
{
private:
    /* data */
public:
    unordered_map<uint16_t, sockpp::tcp_connector> &connectors_map;
    unordered_map<uint16_t, sockpp::tcp_socket> &sockets_map;
    sockpp::tcp_acceptor &acceptor;
    unordered_map<uint16_t, mutex *> mtxs_map;

    MessageQueue<Command> &cmd_dist_queue;

    CmdDist(unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, sockpp::tcp_acceptor &_acceptor, MessageQueue<Command> &_cmd_dist_queue, unsigned int _num_threads);
    ~CmdDist();

    void run() override;
};

#endif // __CMD_DIST_HH__