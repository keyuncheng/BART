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
    // table
    unsigned char *re_matrix;
    unsigned char *re_encode_gftbl;

    unsigned char **pm_matrix;
    unsigned char **pm_encode_gftbl;

    ComputeWorker(Config &_config, MessageQueue<ParityComputeTask> &_parity_compute_queue, MemoryPool &_memory_pool, unsigned _num_threads);
    ~ComputeWorker();

    Config &config;
    MessageQueue<ParityComputeTask> &parity_compute_queue;
    MemoryPool &memory_pool;

    // on-going parity computation task map
    unordered_map<string, ParityComputeTask> ongoing_task_map;
    mutex ongoing_task_map_mtx;

    void run() override;

    void initECTables();
    void destroyECTables();

    unsigned char gfPow(unsigned char val, unsigned int times);

    string getParityComputeTaskKey(EncodeMethod enc_method, uint32_t post_stripe_id, uint8_t post_block_id);
    bool isTaskOngoing(EncodeMethod enc_method, uint32_t post_stripe_id, uint8_t post_block_id);
};

#endif // __COMPUTE_WORKER_HH__