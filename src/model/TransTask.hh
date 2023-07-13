#ifndef __TRANS_TASK_HH__
#define __TRANS_TASK_HH__

#include "../include/include.hh"

enum TransTaskType
{
    // parity computation: re-encoding
    READ_RE_BLK,
    TRANSFER_COMPUTE_RE_BLK,
    COMPUTE_RE_BLK,
    // parity computation: parity merging
    READ_PM_BLK,
    TRANSFER_COMPUTE_PM_BLK,
    COMPUTE_PM_BLK,
    // block relocation
    READ_RELOC_BLK,
    TRANSFER_RELOC_BLK,
    // write
    WRITE_BLK,
    // delete
    DELETE_BLK,
    UNKNOWN
};

class TransTask
{
private:
    /* data */
public:
    TransTaskType type;             // task type
    uint32_t post_stripe_id;        // post-transition stripe id (or stripe group id)
    uint8_t post_block_id;          // block id in the post-transition stripe
    uint32_t pre_stripe_id_global;  // pre-transition stripe in the stripe batch
    uint8_t pre_stripe_id_relative; // pre-transition stripe id in the stripe group
    uint8_t pre_block_id;           // block id in stripe group
    uint16_t src_node_id;           // source node id
    uint16_t dst_node_id;           // destination node id

    TransTask(TransTaskType _type, uint32_t _post_stripe_id, uint8_t _post_block_id, uint32_t _pre_stripe_id_global, uint8_t _pre_stripe_id_relative, uint8_t _pre_block_id, uint16_t _src_node_id, uint16_t _dst_node_id);
    ~TransTask();

    /**
     * @brief print task
     *
     */
    void print();
};

#endif // __TRANS_TASK_HH__