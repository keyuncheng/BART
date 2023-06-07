#ifndef __CMD_HANDLER_HH__
#define __CMD_HANDLER_HH__

#include <mutex>
#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include "../include/include.hh"
#include "../util/Utils.hh"
#include "../util/ThreadPool.hh"
#include "../util/MessageQueue.hh"
#include "../util/Config.hh"
#include "BlockIO.hh"
#include "Command.hh"

class CmdHandler : public ThreadPool
{
private:
    /* data */
public:
    Config &config;
    unordered_map<uint16_t, sockpp::tcp_socket> &sockets_map;
    unordered_map<uint16_t, mutex *> mtxs_map;
    unordered_map<uint16_t, thread *> handler_threads_map;

    MessageQueue<Command> &cmd_dist_queue;

    unsigned char *block_buffer;

    CmdHandler(Config &_config, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, MessageQueue<Command> &_cmd_dist_queue, unsigned int _num_threads);
    ~CmdHandler();

    void run() override;

    void handleControllerCmd();
    void handleAgentCmd(uint16_t conn_id);
};

#endif // __CMD_HANDLER_HH__