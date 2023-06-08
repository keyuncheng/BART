#ifndef __COMPUTE_WORKER_HH__
#define __COMPUTE_WORKER_HH__

#include <mutex>
#include "../include/include.hh"
#include "../util/ThreadPool.hh"
#include "ParityComputeTask.hh"
#include "../util/MessageQueue.hh"
#include "../util/Config.hh"
#include "../util/MemoryPool.hh"

class ComputeWorker : public ThreadPool
{
private:
    /* data */
public:
    ComputeWorker(Config &_config, MessageQueue<ParityComputeTask> &_parity_compute_queue, MemoryPool &_memory_pool);
    ~ComputeWorker();

    Config &config;
    MessageQueue<ParityComputeTask> &parity_compute_queue;
    MemoryPool &memory_pool;

    mutex ongoing_task_map_mtx;

    // key: post_stripe_id:encode_method:parity_id
    // Note: for re-encoding,
    map<string, ParityComputeTask> ongoing_task_map;

    void run() override;

    void computeReencode(uint8_t parity_block_id, uint8_t data_block_id, unsigned char *buffer);
    void computeParityMerging(uint8_t parity_block_id, uint8_t data_block_id, unsigned char *buffer);
};

#endif // __COMPUTE_WORKER_HH__