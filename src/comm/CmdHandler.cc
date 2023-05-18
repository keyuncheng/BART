#include "CmdHandler.hh"

CmdHandler::CmdHandler(unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, sockpp::tcp_acceptor &_acceptor, unsigned int _num_threads) : ThreadPool<Command>(_num_threads), connectors_map(_connectors_map), sockets_map(_sockets_map), acceptor(_acceptor)
{
    // init mutex
    for (auto &item : connectors_map)
    {
        mtxs_map[item.first] = new mutex();
    }
}

CmdHandler::~CmdHandler()
{
    // init mutex
    for (auto &item : mtxs_map)
    {
        delete mtxs_map[item.first];
    }
}

void CmdHandler::run()
{
    // TO IMPLEMENT
}