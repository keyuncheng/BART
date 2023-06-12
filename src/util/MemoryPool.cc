#include "MemoryPool.hh"

MemoryPool::MemoryPool(unsigned int _num_blocks, uint64_t _block_size) : num_blocks(_num_blocks), block_size(_block_size)
{
    // free block list
    // free_block_queue = new moodycamel::ConcurrentQueue<unsigned int>(num_blocks);

    free_block_queue = new queue<unsigned int>();

    block_ptrs = (unsigned char **)malloc(num_blocks * sizeof(unsigned char *));
    for (unsigned int block_id = 0; block_id < num_blocks; block_id++)
    {
        // allocate block in memory
        block_ptrs[block_id] = (unsigned char *)malloc(block_size * sizeof(unsigned char));

        // create block ptr map
        block_ptrs_map[block_ptrs[block_id]] = block_id;

        // add to free block
        // free_block_queue->enqueue(block_id);

        free_block_queue->push(block_id);
    }
}

MemoryPool::~MemoryPool()
{
    for (unsigned int block_id = 0; block_id < num_blocks; block_id++)
    {
        free(block_ptrs[block_id]);
    }
    free(block_ptrs);

    delete free_block_queue;
}

unsigned char *MemoryPool::getBlock()
{
    unsigned int free_block_id;
    unique_lock<mutex> lck(free_queue_mutex);
    while (free_block_queue->empty())
    {
        queue_cv.wait(lck, [&]
                      { return !free_block_queue->empty(); });
    }
    // // dequeue and return free block
    // if (free_block_queue->try_dequeue(free_block_id) == true)
    // {
    //     return block_ptrs[free_block_id];
    // }

    free_block_id = free_block_queue->front();
    free_block_queue->pop();
    return block_ptrs[free_block_id];
}

void MemoryPool::freeBlock(unsigned char *block_ptr)
{
    auto it = block_ptrs_map.find(block_ptr);
    if (it == block_ptrs_map.end())
    {
        fprintf(stderr, "error: invalid block pointer to free: %p\n", block_ptr);
        exit(EXIT_FAILURE);
    }

    // enqueue the free block
    // free_block_queue->enqueue(it->second);

    unique_lock<mutex> lck(free_queue_mutex);
    free_block_queue->push(it->second);
    lck.unlock();
    queue_cv.notify_one();
}