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
    uint16_t self_conn_id;
    unordered_map<uint16_t, sockpp::tcp_socket> &sockets_map;

    // command distribution queues: each retrieves command from CmdHandler and distributes commands to the corresponding CmdDist (CmdHandler -> CmdDist)
    unordered_map<uint16_t, MessageQueue<Command> *> *cmd_dist_queues;

    // parity compute task queue: each retrieves parity computation task from Controller and pass to ComputeWorker (CmdHandler -> ComputeWorker)
    unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> *pc_task_queues;

    // parity compute result queue: each retrieves parity computation result from ComputeWorker and pass to CmdHandler (ComputeWorker -> CmdHandler)
    unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> *pc_reply_queues;

    // parity relocation task queue (ComputeWorker -> CmdHandler)
    MessageQueue<ParityComputeTask> *parity_reloc_task_queue;

    // memory pool (for allocating blocks for parity computation)
    MemoryPool *memory_pool;

    // mutex for hand1lers
    mutex ready_mtx;
    condition_variable ready_cv;
    atomic<bool> is_ready;

    // command handler threads
    unordered_map<uint16_t, thread *> handler_threads_map;

    // parity relocation task thread
    thread *parity_reloc_task_thread;

    CmdHandler(Config &_config, uint16_t _self_conn_id,
               unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map,
               unordered_map<uint16_t, MessageQueue<Command> *> *_cmd_dist_queues,
               unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> *_pc_task_queues,
               unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> *_pc_reply_queues,
               MessageQueue<ParityComputeTask> *_pc_reloc_task_queue, MemoryPool *_memory_pool, unsigned int _num_threads);
    ~CmdHandler();

    void run() override;

    // handle command thread
    void handleCmdFromController();
    void handleCmdFromAgent(uint16_t src_conn_id);

    // parity relocation thread
    void handleParityRelocTask();

    void distStopCmds();
};

#endif // __CMD_HANDLER_HH__