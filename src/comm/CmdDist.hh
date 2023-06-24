#ifndef __CMD_DIST_HH__
#define __CMD_DIST_HH__

#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include "../include/include.hh"
#include "../util/Utils.hh"
#include "../util/MessageQueue.hh"
#include "../util/ThreadPool.hh"
#include "../util/Config.hh"
#include "Command.hh"
#include "BlockIO.hh"
#include "../util/MemoryPool.hh"

class CmdDist : public ThreadPool
{
private:
    /* data */
public:
    // config
    Config &config;

    // current connection id
    uint16_t self_conn_id;

    // Node connectors
    unordered_map<uint16_t, sockpp::tcp_connector> &connectors_map;

    // command distribution queues: each retrieves command from CmdHandler and distributes commands to the corresponding CmdDist (CmdHandler -> CmdDist)
    unordered_map<uint16_t, MessageQueue<Command> *> &cmd_dist_queues;

    // command distributor threads
    unordered_map<uint16_t, thread *> dist_threads_map;

    CmdDist(Config &_config, uint16_t _self_conn_id, unordered_map<uint16_t, sockpp::tcp_connector> &_connectors_map, unordered_map<uint16_t, MessageQueue<Command> *> &_cmd_dist_queues);
    ~CmdDist();

    /**
     * @brief thread initializer
     *
     */
    void run() override;

    /**
     * @brief distribute commands to Node <dst_conn_id>
     *
     * @param dst_conn_id destination connection id
     */
    void distCmdToNode(uint16_t dst_conn_id);
};

#endif // __CMD_DIST_HH__