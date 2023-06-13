#ifndef __CMD_HANDLER_HH__
#define __CMD_HANDLER_HH__

#include <mutex>
#include <condition_variable>
#include <atomic>

#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_acceptor.h"
#include "../include/include.hh"
#include "../util/Utils.hh"
#include "../util/ThreadPool.hh"
#include "../util/MessageQueue.hh"
#include "../util/MultiWriterQueue.h"
#include "../util/Config.hh"
#include "ComputeWorker.hh"
#include "BlockIO.hh"
#include "Command.hh"
#include "ParityComputeTask.hh"
#include "../util/MemoryPool.hh"

class CmdHandler : public ThreadPool
{
private:
    /* data */
public:
    Config &config;
    unordered_map<uint16_t, sockpp::tcp_socket> &sockets_map;

    // handler threads
    unordered_map<uint16_t, thread *> handler_threads_map;

    mutex ready_mtx;
    condition_variable ready_cv;
    atomic<bool> is_ready;

    // message queues for distributing commands
    // each command handler thread pushes commands to the corresponding queue
    unordered_map<uint16_t, MessageQueue<Command> *> *cmd_dist_queues;

    // parity compute queue (parse parity computation commands and push the parity compute task to ComputeWorker)
    MultiWriterQueue<ParityComputeTask> *parity_compute_queue;

    // memory pool (for allocating blocks for parity computation)
    MemoryPool *memory_pool;

    // used for retrieving parity block computation status only
    ComputeWorker *compute_worker;

    CmdHandler(Config &_config, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, unordered_map<uint16_t, MessageQueue<Command> *> *_cmd_dist_queues, MultiWriterQueue<ParityComputeTask> *_parity_compute_queue, MemoryPool *_memory_pool, ComputeWorker *_compute_worker, unsigned int _num_threads);
    ~CmdHandler();

    void run() override;

    void handleCmdFromController();
    void handleCmdFromAgent(uint16_t src_conn_id);
};

#endif // __CMD_HANDLER_HH__