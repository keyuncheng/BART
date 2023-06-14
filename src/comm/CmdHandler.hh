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
    uint16_t self_conn_id;
    unordered_map<uint16_t, sockpp::tcp_socket> &sockets_map;

    // mutex for handlers
    mutex ready_mtx;
    condition_variable ready_cv;
    atomic<bool> is_ready;

    // command handler threads
    unordered_map<uint16_t, thread *> handler_threads_map;

    // parity relocation command handler
    thread *parity_reloc_thread;

    // handle parity relocation command
    mutex parity_reloc_cmd_map_mtx;
    unordered_map<uint32_t, vector<Command>> parity_reloc_cmd_map;

    // command distribution queues (each is a single reader single writer queue) (CmdDist <-> CmdHandler)
    unordered_map<uint16_t, MessageQueue<Command> *> *cmd_dist_queues;

    // parity compute queue (a multiple writer single reader queue) (CmdHandler <-> ComputeWorker)
    MultiWriterQueue<ParityComputeTask> *parity_compute_queue;

    // parity compute result queue (ComputeWorker <-> CmdHandler)
    MessageQueue<string> *parity_comp_result_queue;

    // memory pool (for allocating blocks for parity computation)
    MemoryPool *memory_pool;

    // used for retrieving parity block computation status only
    ComputeWorker *compute_worker;

    CmdHandler(Config &_config, uint16_t _self_conn_id, unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map, unordered_map<uint16_t, MessageQueue<Command> *> *_cmd_dist_queues, MultiWriterQueue<ParityComputeTask> *_parity_compute_queue, MessageQueue<string> *_parity_comp_result_queue, MemoryPool *_memory_pool, ComputeWorker *_compute_worker, unsigned int _num_threads);
    ~CmdHandler();

    void run() override;

    void handleCmdFromController();
    void handleCmdFromAgent(uint16_t src_conn_id);

    // handle parity relocation command
    void handleParityRelocCmdFromController();
};

#endif // __CMD_HANDLER_HH__