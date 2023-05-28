#include "TransTask.hh"

TransTask::TransTask(TransTaskType _type, uint32_t _post_stripe_id, uint8_t _post_block_id, uint32_t _pre_stripe_id_global, uint8_t _pre_stripe_id_relative, uint8_t _pre_block_id, uint16_t _src_node_id, uint16_t _dst_node_id) : type(_type), post_stripe_id(_post_stripe_id), post_block_id(_post_block_id), pre_stripe_id_global(_pre_stripe_id_global), pre_stripe_id_relative(_pre_stripe_id_relative), pre_block_id(_pre_block_id), src_node_id(_src_node_id), dst_node_id(_dst_node_id)
{
}

TransTask::~TransTask()
{
}

void TransTask::print()
{
    printf("Transition Task: type: %u, post_stripe: (%u, %u), pre_stripe: (%u, %u, %u), src_node: %u, dst_node: %u\n", type, post_stripe_id, post_block_id, pre_stripe_id_global, pre_stripe_id_relative, pre_block_id, src_node_id, dst_node_id);
}