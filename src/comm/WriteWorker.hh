#ifndef __WRITE_WORKER_HH__
#define __WRITE_WORKER_HH__

#include "../include/include.hh"
#include "../util/ThreadPool.hh"

class WriteWorker : public ThreadPool
{
private:
    /* data */
public:
    WriteWorker(/* args */);
    ~WriteWorker();
};

#endif // __WRITE_WORKER_HH__