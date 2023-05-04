#ifndef __TRANS_TASK_HH__
#define __TRANS_TASK_HH__

#include "../include/include.hh"

enum TransTaskType
{
    READ_BLK,
    WRITE_BLK,
    DELETE_BLK,
    TRANSFER_BLK,
    COMPUTE_BLK,
    UNKNOWN
};

class TransTask
{
private:
    /* data */
public:
    TransTaskType type;
    uint32_t sg_id;            // stripe group id
    uint32_t stripe_id_global; // stripe id in stripe batch
    uint8_t stripe_id;         // stripe id in stripe group
    uint8_t block_id;          // block id in stripe group
    uint16_t node_id;          // node id

    // move block task
    uint16_t dst_node_id;

    TransTask(TransTaskType _type, uint32_t _sg_id, uint32_t _stripe_id_global, uint8_t _stripe_id, uint8_t _block_id, uint16_t _node_id);
    ~TransTask();

    void print();
};

#endif // __TRANS_TASK_HH__