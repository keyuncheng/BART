#include "TransTask.hh"

TransTask::TransTask(TransTaskType _type, uint32_t _sg_id, uint32_t _stripe_id_global, uint8_t _stripe_id, uint8_t _block_id, uint16_t _node_id) : type(_type), sg_id(_sg_id), stripe_id_global(_stripe_id_global), stripe_id(_stripe_id), block_id(_block_id), node_id(_node_id)
{
}

TransTask::~TransTask()
{
}

void TransTask::print()
{
    string task_name;
    switch (type)
    {
    case TransTaskType::READ_BLK:
        task_name = "READ_BLK";
        break;
    case TransTaskType::WRITE_BLK:
        task_name = "WRITE_BLK";
        break;
    case TransTaskType::TRANSFER_BLK:
        task_name = "TRANSFER_BLK";
        break;
    case TransTaskType::COMPUTE_BLK:
        task_name = "COMPUTE_BLK";
        break;
    case TransTaskType::DELETE_BLK:
        task_name = "DELETE_BLK";
        break;
    default:
        task_name = "UNKNOWN";
        break;
    }

    printf("Transition Task: type: %s, stripe_group_id: %u, stripe_id_global: %u, stripe_id: %u, block_id: %u, node_id: %u", task_name.c_str(), sg_id, stripe_id_global, stripe_id, block_id, node_id);

    if (type == TransTaskType::TRANSFER_BLK)
    {
        printf(", dst_node_id: %u", dst_node_id);
    }

    printf("\n");
}