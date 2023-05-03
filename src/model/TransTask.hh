#ifndef __STRIPE_MERGE_G_HH__
#define __STRIPE_MERGE_G_HH__

#include "../include/include.hh"

enum TransTaskType
{
    READ_BLK,
    WRITE_BLK,
    DELETE_BLK,
    SEND_BLK,
    COMPUTE_BLK,
};

class TransTask
{
private:
    /* data */
public:
    TransTaskType type;
    uint32_t stripe_batch_id;  // stripe batch id
    uint32_t stripe_id_global; // stripe id in stripe batch
    uint32_t sg_id;            // stripe group id
    uint32_t stripe_id;        // stripe id in stripe group
    uint8_t block_id;          // block id in stripe group

    TransTask(/* args */);
    ~TransTask();
};

#endif // __STRIPE_MERGE_G_HH__