#ifndef __COMPUTE_WORKER_HH__
#define __COMPUTE_WORKER_HH__

#include "../include/include.hh"
#include "../util/ThreadPool.hh"
#include "ParityComputeTask.hh"

class ComputeWorker : public ThreadPool
{
private:
    /* data */
public:
    ComputeWorker(/* args */);
    ~ComputeWorker();

    mutex ongoing_task_map_mtx;
    map<string, ParityComputeTask> ongoing_task_map;

    void run() override;
};

#endif // __COMPUTE_WORKER_HH__