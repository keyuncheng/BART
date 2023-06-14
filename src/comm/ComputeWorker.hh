#ifndef __COMPUTE_WORKER_HH__
#define __COMPUTE_WORKER_HH__

#include <mutex>
#include <isa-l.h>

#include "../include/include.hh"
#include "../util/ThreadPool.hh"
#include "ParityComputeTask.hh"
#include "../util/MessageQueue.hh"
#include "../util/MultiWriterQueue.h"
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

    ComputeWorker(Config &_config, MultiWriterQueue<ParityComputeTask> &_parity_compute_queue, MessageQueue<string> &_parity_comp_result_queue, MemoryPool &_memory_pool, unsigned _num_threads);
    ~ComputeWorker();

    Config &config;

    // parity compute queue (passed from CmdHandler)
    MultiWriterQueue<ParityComputeTask> &parity_compute_queue;

    // parity compute result queue (ComputeWorker <-> CmdHandler)
    MessageQueue<string> &parity_comp_result_queue;

    // memory pool (passed from CmdHandler, used to free blocks only)
    MemoryPool &memory_pool;

    // on-going parity computation task map
    unordered_map<string, ParityComputeTask> ongoing_task_map;

    void run() override;

    void initECTables();
    void destroyECTables();

    unsigned char gfPow(unsigned char val, unsigned int times);

    static string getParityComputeTaskKey(EncodeMethod enc_method, uint32_t post_stripe_id, uint8_t post_block_id);

    static void getParityComputeTaskFromKey(string key, EncodeMethod &enc_method, uint32_t &post_stripe_id, uint8_t &post_block_id);
};

#endif // __COMPUTE_WORKER_HH__