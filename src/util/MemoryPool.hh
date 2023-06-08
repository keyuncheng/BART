#ifndef __MEMORY_POOL_HH__
#define __MEMORY_POOL_HH__

#include <mutex>
#include <condition_variable>

#include "../include/include.hh"
#include "../util/concurrentqueue.h"

class MemoryPool
{
private:
    /* data */
public:
    unsigned int num_blocks;
    uint64_t block_size;
    unsigned char **block_ptrs;

    moodycamel::ConcurrentQueue<unsigned int> *free_block_queue;
    unordered_map<unsigned char *, unsigned int> block_ptrs_map;

    MemoryPool(unsigned int _num_blocks, uint64_t _block_size);
    ~MemoryPool();

    unsigned char *getBlock();
    void freeBlock(unsigned char *block_ptr);
};

#endif // __MEMORY_POOL_HH__