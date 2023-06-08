#include "MemoryPool.hh"

MemoryPool::MemoryPool(unsigned int _num_blocks, uint64_t _block_size) : num_blocks(_num_blocks), block_size(_block_size)
{
    // free block list
    free_block_queue = new MessageQueue<unsigned int>(num_blocks);

    block_ptrs = (unsigned char **)malloc(block_size * sizeof(unsigned char *));
    for (unsigned int block_id = 0; block_id < num_blocks; block_id++)
    {
        // allocate block in memory
        block_ptrs[block_id] = (unsigned char *)malloc(block_size * sizeof(unsigned char));

        // create block ptr map
        block_ptrs_map[block_ptrs[block_id]] = block_id;

        // add to free block
        free_block_queue->Push(block_id);
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
    unsigned free_block_id;
    while (true)
    {
        // wait until there is a free block
        if (free_block_queue->IsEmpty() == true)
        {
            continue;
        }

        // dequeue and return free block
        if (free_block_queue->Pop(free_block_id) == true)
        {
            return block_ptrs[free_block_id];
        }
    }
}

void MemoryPool::freeBlock(unsigned char *block_ptr)
{
    auto it = block_ptrs_map.find(block_ptr);
    if (it != block_ptrs_map.end())
    {
        fprintf(stderr, "error: invalid block pointer to free: %x\n", block_ptr);
        return;
    }

    // enqueue the free block
    free_block_queue->Push(it->second);
}