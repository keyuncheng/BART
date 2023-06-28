#ifndef __CMD_HANDLER_HH__
#define __CMD_HANDLER_HH__

#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "../util/Config.hh"
#include "../util/ThreadPool.hh"
#include "../util/MessageQueue.hh"
#include "../util/MultiWriterQueue.h"
#include "Command.hh"
#include "BlockIO.hh"

class CmdHandler : public ThreadPool
{
private:
    /* data */
public:
    // config
    Config &config;

    // current connection id
    uint16_t self_conn_id;

    // Node sockets
    unordered_map<uint16_t, sockpp::tcp_socket> &sockets_map;

    // command distribution queues: each retrieves command from CmdHandler and distributes commands to the corresponding CmdDist (CmdHandler -> CmdDist)
    unordered_map<uint16_t, MessageQueue<Command> *> *cmd_dist_queues;

    // compute task queues: each retrieves computation task from Controller, and pass to a ComputeWorker (CmdHandler -> ComputeWorker<worker_id>)
    unordered_map<unsigned int, MessageQueue<Command> *> *compute_task_queues;

    // parity block relocation task queue: each retrieves block relocation task (from both CmdHandler and ComputeWorker), and pass to a RelocWorker (ComputerWorker / CmdHandler -> RelocWorker<worker_id>)
    unordered_map<unsigned int, MultiWriterQueue<Command> *> *reloc_task_queues;

    // command handler threads
    unordered_map<uint16_t, thread *> handler_threads_map;

    CmdHandler(Config &_config, uint16_t _self_conn_id,
               unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map,
               unordered_map<uint16_t, MessageQueue<Command> *> *_cmd_dist_queues,
               unordered_map<unsigned int, MessageQueue<Command> *> *_compute_task_queues,
               unordered_map<unsigned int, MultiWriterQueue<Command> *> *_reloc_task_queues);
    ~CmdHandler();

    /**
     * @brief thread initializer
     *
     */
    void run() override;

    /**
     * @brief handle commands from Controller
     *
     */
    void handleCmdFromController();

    /**
     * @brief handle commands from Agent <src_conn_id>
     *
     * @param src_conn_id source connection id
     */
    void handleCmdFromAgent(uint16_t src_conn_id);

    void distStopCmds();
};

#endif // __CMD_HANDLER_HH__