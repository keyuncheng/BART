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

    void initECTables();
    void destroyECTables();

    unsigned char gfPow(unsigned char val, unsigned int times);
};

#endif // __COMPUTE_WORKER_HH__