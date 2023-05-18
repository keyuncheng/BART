#ifndef __CMD_HANDLER_HH__
#define __CMD_HANDLER_HH__

#include <mutex>
#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include "../include/include.hh"
#include "../util/ThreadPool.hh"
#include "Command.hh"

class CmdHandler : public ThreadPool<Command>
{
private:
    /* data */
public:
    unordered_map<uint16_t, sockpp::tcp_connector> &connectors_map;
    unordered_map<uint16_t, sockpp::tcp_socket> &sockets_map;
    sockpp::tcp_acceptor &acceptor;

    unordered_map<uint16_t, mutex *> mtxs_map;

    CmdHandler(unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, sockpp::tcp_acceptor &_acceptor, unsigned int _num_threads);
    ~CmdHandler();

    void run() override;
};

#endif // __CMD_HANDLER_HH__