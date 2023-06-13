#ifndef __CMD_DIST_HH__
#define __CMD_DIST_HH__

#include <mutex>
#include <condition_variable>
#include <atomic>

#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include "../include/include.hh"
#include "../util/Utils.hh"
#include "../util/MessageQueue.hh"
#include "../util/ThreadPool.hh"
#include "../util/Config.hh"
#include "Command.hh"
#include "BlockIO.hh"

class CmdDist : public ThreadPool
{
private:
    /* data */
public:
    Config &config;
    unordered_map<uint16_t, sockpp::tcp_connector> &connectors_map;

    // distributor threads
    unordered_map<uint16_t, thread *> dist_threads_map;

    // ready to work
    mutex ready_mtx;
    condition_variable ready_cv;
    atomic<bool> is_ready;

    // message queues for distributing commands (each distributor thread pops commands from the corresponding queue)
    unordered_map<uint16_t, MessageQueue<Command> *> &cmd_dist_queues;

    CmdDist(Config &_config, unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, MessageQueue<Command> *> &_cmd_dist_queues, unsigned int _num_threads);
    ~CmdDist();

    void run() override;

    void distCmdToController();
    void distCmdToAgent(uint16_t dst_conn_id);
};

#endif // __CMD_DIST_HH__