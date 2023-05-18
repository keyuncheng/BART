#include "CmdDist.hh"

CmdDist::CmdDist(unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, sockpp::tcp_acceptor &_acceptor, unsigned int _num_threads) : ThreadPool<Command>(_num_threads), connectors_map(_connectors_map), sockets_map(_sockets_map), acceptor(_acceptor)
{
    // init mutex
    for (auto &item : connectors_map)
    {
        mtxs_map[item.first] = new mutex();
    }
}

CmdDist::~CmdDist()
{
    // init mutex
    for (auto &item : mtxs_map)
    {
        delete mtxs_map[item.first];
    }
}

void CmdDist::run()
{
    Command cmd;
    while (true)
    {
        if (queue->Pop(cmd) == true)
        {
            uint16_t dst_node_id = cmd.dst_conn_id;
            unique_lock<mutex> lck(*mtxs_map[dst_node_id]); // add lock
            auto &connector = connectors_map[dst_node_id];

            // send the command
            if (connector.write_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "error send cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }
        }
    }
}