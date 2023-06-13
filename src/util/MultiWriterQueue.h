#ifndef __MULTIWRITER_QUEUE_HH__
#define __MULTIWRITER_QUEUE_HH__

#include "../include/include.hh"
// #include "concurrentqueue.h"
#include "blockingconcurrentqueue.h"
#include <mutex>
#include <queue>

template <class T>
class MultiWriterQueue
{
private:
    /* data */
public:
    // use the open-source lock-free queue from here: https://github.com/cameron314/readerwriterqueue
    // moodycamel::MultiWriterQueue<T> *queue;
    moodycamel::BlockingConcurrentQueue<T> *queue;

    /**
     * @brief Construct a new MultiWriter Queue object
     *
     * @param maxSize
     */
    MultiWriterQueue(uint32_t maxSize)
    {
        // queue = new moodycamel::ReaderWriterQueue<T>(maxSize);
        // queue = new moodycamel::MultiWriterQueue<T>(maxSize);
        queue = new moodycamel::BlockingConcurrentQueue<T>(maxSize);
    }

    /**
     * @brief Destroy the MultiWriter Queue object
     *
     */
    ~MultiWriterQueue()
    {
        bool flag = IsEmpty();
        if (flag)
        {
            delete queue;
        }
        else
        {
            fprintf(stderr, "MultiWriterQueue: Queue is not empty.\n");
            exit(EXIT_FAILURE);
        }
    }

    /**
     * @brief Push data to the queue
     *
     * @param data
     * @return true
     * @return false
     */
    bool Push(T &data)
    {
        while (!queue->try_enqueue(data))
            ;
        return true;
    }

    /**
     * @brief Pop data from the queue
     *
     * @param data
     * @return true
     * @return false
     */
    bool Pop(T &data)
    {
        return queue->try_dequeue(data);
    }

    /**
     * @brief Check whether the queue is empty
     *
     * @return true
     * @return false
     */
    bool IsEmpty()
    {
        size_t count = queue->size_approx();
        return (count == 0);
    }
};

#endif // __MULTIWRITER_QUEUE_HH__