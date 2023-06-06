#ifndef __THREAD_POOL_HH__
#define __THREAD_POOL_HH__

#include <thread>
#include <atomic>
#include "../include/include.hh"
#include "MessageQueue.hh"

template <class T>
class ThreadPool
{
private:
    /* data */
public:
    unsigned int num_threads; // number of threads
    std::thread *threads;     // threads
    atomic<bool> is_finished; // check whether the tasks has been finished
    MessageQueue<T> *queue;   // message queue

    ThreadPool(unsigned int _num_threads = 1)
    {
        num_threads = _num_threads;
        threads = new thread[num_threads];
        is_finished = false;
        queue = new MessageQueue<T>(MAX_MSG_QUEUE_LEN);
    }

    virtual ~ThreadPool()
    {
        delete queue;
        if (num_threads > 0)
        {
            delete[] threads;
        }
    }

    virtual void run() = 0; // need to implement

    void start()
    {
        // sequentially start the threads
        for (unsigned int thread_id = 0; thread_id < num_threads; thread_id++)
        {
            threads[thread_id] = thread([&]
                                        { this->run(); });
        }
    }

    void wait()
    {
        // sequentially wait for threads to join
        for (unsigned int thread_id = 0; thread_id < num_threads; thread_id++)
        {
            threads[thread_id].join();
        }
    }

    bool finished()
    {
        return (is_finished == true && queue->IsEmpty() == true);
    }

    void set_finished()
    {
        is_finished = true;
    }
};

#endif // __THREAD_POOL_HH__