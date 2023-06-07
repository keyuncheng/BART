#include "CmdDist.hh"

CmdDist::CmdDist(unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, sockpp::tcp_acceptor &_acceptor, MessageQueue<Command> &_cmd_dist_queue, unsigned int _num_threads) : ThreadPool(_num_threads), connectors_map(_connectors_map), sockets_map(_sockets_map), acceptor(_acceptor), cmd_dist_queue(_cmd_dist_queue)
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
    printf("start to distribute command\n");
    while (finished() == false || cmd_dist_queue.IsEmpty() == false)
    {
        Command cmd;
        if (cmd_dist_queue.Pop(cmd) == true)
        {
            unique_lock<mutex> lck(*mtxs_map[cmd.dst_conn_id]); // add lock
            auto &connector = connectors_map[cmd.dst_conn_id];
            // send the command
            if (connector.write_n(cmd.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "error send cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd.type, cmd.src_conn_id, cmd.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            cmd.print();
        }
    }
}