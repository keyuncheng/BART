#ifndef __COMPUTE_WORKER_HH__
#define __COMPUTE_WORKER_HH__

#include <mutex>
#include <isa-l.h>

#include "../include/include.hh"
#include "../util/ThreadPool.hh"
#include "ParityComputeTask.hh"
#include "../util/MessageQueue.hh"
#include "../util/Config.hh"
#include "../util/MemoryPool.hh"
#include "BlockIO.hh"

class ComputeWorker : public ThreadPool
{
private:
    /* data */
public:
    Config &config;

    uint16_t self_conn_id;

    unordered_map<uint16_t, sockpp::tcp_socket> &sockets_map;

    // parity compute task queue: each retrieves parity computation task from Controller and pass to ComputeWorker (CmdHandler -> ComputeWorker)
    unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> &pc_task_queues;

    // parity compute result queue: each retrieves parity computation result from ComputeWorker and pass to CmdHandler (ComputeWorker -> CmdHandler)
    unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> &pc_reply_queues;

    MessageQueue<ParityComputeTask> &parity_reloc_task_queue;

    // memory pool (passed from CmdHandler, used to free blocks only)
    MemoryPool &memory_pool;

    // table
    unsigned char *re_matrix;
    unsigned char *re_encode_gftbl;

    unsigned char **pm_matrix;
    unsigned char **pm_encode_gftbl;

    // parity buffer for re-encoding and parity merging
    unsigned char *buffer;
    unsigned char **re_buffers;
    unsigned char **pm_buffers;

    ComputeWorker(Config &_config, uint16_t _self_conn_id,
                  unordered_map<uint16_t, sockpp::tcp_socket> &_sockets_map,
                  unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> &_pc_task_queues,
                  unordered_map<uint16_t, MessageQueue<ParityComputeTask> *> &_pc_reply_queues,
                  MessageQueue<ParityComputeTask> &_parity_reloc_task_queue,
                  MemoryPool &_memory_pool,
                  unsigned _num_threads);
    ~ComputeWorker();

    void run() override;

    void initECTables();
    void destroyECTables();

    unsigned char gfPow(unsigned char val, unsigned int times);

    void retrieveDataAndReply(ParityComputeTask &parity_compute_task, uint16_t src_node_id, unsigned char *buffer);
};

#endif // __COMPUTE_WORKER_HH__