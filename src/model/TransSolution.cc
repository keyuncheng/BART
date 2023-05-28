#include "TransSolution.hh"

TransSolution::TransSolution(ConvertibleCode &_code, ClusterSettings &_settings) : code(_code), settings(_settings)
{
    init();
}

TransSolution::~TransSolution()
{
    destroy();
}

void TransSolution::init()
{
    sg_tasks.clear();
    sg_transfer_tasks.clear();

    if (TRANSFER_TASKS_ONLY == false)
    {
        sg_read_tasks.clear();
        sg_write_tasks.clear();
        sg_delete_tasks.clear();
        sg_compute_tasks.clear();
    }
}

void TransSolution::destroy()
{
    for (auto &item : sg_tasks)
    {
        for (auto &task : item.second)
        {
            delete task;
        }
    }
}

void TransSolution::print()
{
    map<uint32_t, vector<TransTask *>> *task_list;
    if (TRANSFER_TASKS_ONLY == true)
    {
        task_list = &sg_transfer_tasks;
    }
    else
    {
        task_list = &sg_tasks;
    }

    for (auto &item : *task_list)
    {
        printf("stripe group %u, number of tasks: %lu\n", item.first, item.second.size());
        for (auto &task : item.second)
        {
            task->print();
        }
    }
}

vector<u32string> TransSolution::getTransferLoadDist()
{
    // send and receive load distribution
    // idx 0: send; idx 1: receive
    vector<u32string> load_dist(2, u32string(settings.num_nodes, 0));

    for (auto &item : sg_transfer_tasks)
    {

        for (auto &task : item.second)
        {
            load_dist[0][task->src_node_id]++;
            load_dist[1][task->dst_node_id]++;
        }
    }

    return load_dist;
}

bool TransSolution::isFinalBlockPlacementValid(StripeBatch &stripe_batch)
{

    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;
        u16string &final_block_placement = stripe_group.post_stripe->indices;

        vector<bool> is_node_placed(settings.num_nodes, false);

        for (uint8_t final_block_id = 0; final_block_id < code.n_f; final_block_id++)
        {
            uint16_t placed_node_id = final_block_placement[final_block_id];
            if (is_node_placed[placed_node_id] == false)
            { // mark the node as placed
                is_node_placed[placed_node_id] = true;
            }
            else
            { // more than one block placed at the node
                return false;
            }
        }
    }

    return true;
}

void TransSolution::buildTransTasks(StripeBatch &stripe_batch)
{
    ConvertibleCode &code = stripe_batch.code;
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;

        // 1: generate tasks for parity computation
        if (stripe_group.parity_comp_method == EncodeMethod::RE_ENCODE)
        { // 1. Re-encoding
            uint16_t enc_node = stripe_group.parity_comp_nodes[0];

            // read code.k_f data blocks
            for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
            {
                Stripe *stripe = stripe_group.pre_stripes[stripe_id];
                for (uint8_t data_block_id = 0; data_block_id < code.k_i; data_block_id++)
                {
                    uint16_t data_node_id = stripe->indices[data_block_id];

                    // 1.1 create data block read tasks
                    TransTask *read_task = new TransTask(TransTaskType::READ_RE_BLK, stripe_group.id, INVALID_BLK_ID, stripe->id, stripe_id, data_block_id, data_node_id, data_node_id);
                    sg_tasks[stripe_group.id].push_back(read_task);
                    sg_read_tasks[stripe_group.id].push_back(read_task);

                    // 1.2 create transfer task
                    // check if the parity block is not located at min_bw_node
                    if (data_node_id != enc_node)
                    {
                        TransTask *transfer_task = new TransTask(TransTaskType::TRANSFER_COMPUTE_RE_BLK, stripe_group.id, INVALID_BLK_ID, stripe->id, stripe_id, data_block_id, data_node_id, enc_node);
                        sg_tasks[stripe_group.id].push_back(transfer_task);
                        sg_transfer_tasks[stripe_group.id].push_back(transfer_task);
                    }
                }
            }

            for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
            {
                // 1.3 create compute task
                TransTask *compute_task = new TransTask(TransTaskType::COMPUTE_RE_BLK, stripe_group.id, code.k_f + parity_id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, INVALID_BLK_ID, enc_node, enc_node);
                sg_tasks[stripe_group.id].push_back(compute_task);
                sg_compute_tasks[stripe_group.id].push_back(compute_task);

                // 1.4 create write task
                TransTask *write_task = new TransTask(TransTaskType::WRITE_BLK, stripe_group.id, code.k_f + parity_id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, INVALID_BLK_ID, enc_node, enc_node);
                sg_tasks[stripe_group.id].push_back(write_task);
                sg_write_tasks[stripe_group.id].push_back(write_task);
            }
        }
        else if (stripe_group.parity_comp_method == EncodeMethod::PARITY_MERGE)
        { // 2. Parity Merging
            for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
            {
                uint16_t enc_node = stripe_group.parity_comp_nodes[parity_id];

                for (uint32_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
                {
                    Stripe *stripe = stripe_group.pre_stripes[stripe_id];
                    uint16_t parity_node_id = stripe->indices[code.k_i + parity_id];

                    // 2.1 create parity block read tasks
                    TransTask *read_task = new TransTask(TransTaskType::READ_PM_BLK, stripe_group.id, code.k_f + parity_id, stripe->id, stripe_id, code.k_i + parity_id, parity_node_id, parity_node_id);
                    sg_tasks[stripe_group.id].push_back(read_task);
                    sg_read_tasks[stripe_group.id].push_back(read_task);

                    // 2.2 create transfer task
                    // check if the parity block is not located at enc_node
                    if (parity_node_id != enc_node)
                    {
                        TransTask *transfer_task = new TransTask(TransTaskType::TRANSFER_COMPUTE_PM_BLK, stripe_group.id, code.k_f + parity_id, stripe->id, stripe_id, code.k_i + parity_id, parity_node_id, enc_node);
                        sg_tasks[stripe_group.id].push_back(transfer_task);
                        sg_transfer_tasks[stripe_group.id].push_back(transfer_task);
                    }
                }

                // 2.3 create compute task
                TransTask *compute_task = new TransTask(TransTaskType::COMPUTE_PM_BLK, stripe_group.id, code.k_f + parity_id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, code.k_f + parity_id, enc_node, enc_node);
                sg_tasks[stripe_group.id].push_back(compute_task);
                sg_compute_tasks[stripe_group.id].push_back(compute_task);

                // 2.4 create write task
                TransTask *write_task = new TransTask(TransTaskType::WRITE_BLK, stripe_group.id, code.k_f + parity_id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, code.k_f + parity_id, enc_node, enc_node);
                sg_tasks[stripe_group.id].push_back(write_task);
                sg_write_tasks[stripe_group.id].push_back(write_task);
            }
        }

        // 3. create parity block delete task
        for (uint32_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
        {
            Stripe *stripe = stripe_group.pre_stripes[stripe_id];
            for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
            {
                uint16_t parity_node_id = stripe->indices[code.k_i + parity_id];

                // 2.5 create parity block delete tasks
                TransTask *delete_task = new TransTask(TransTaskType::DELETE_BLK, stripe_group.id, INVALID_BLK_ID, stripe->id, stripe_id, code.k_f + parity_id, parity_node_id, parity_node_id);
                sg_tasks[stripe_group.id].push_back(delete_task);
                sg_delete_tasks[stripe_group.id].push_back(delete_task);
            }
        }

        // 3. generate tasks for block relocation
        for (uint8_t final_block_id = 0; final_block_id < code.n_f; final_block_id++)
        {
            uint16_t source_node_id = INVALID_NODE_ID;
            uint16_t dst_node_id = INVALID_NODE_ID;
            uint32_t stripe_id_global = INVALID_STRIPE_ID_GLOBAL;
            uint8_t stripe_id = INVALID_STRIPE_ID;
            uint8_t block_id = INVALID_BLK_ID;

            if (final_block_id < code.k_f)
            { // data block relocation
                stripe_id = final_block_id / code.k_i;
                block_id = final_block_id % code.k_i;
                stripe_id_global = stripe_group.pre_stripes[stripe_id]->id;
                source_node_id = stripe_group.pre_stripes[stripe_id]->indices[block_id];
            }
            else
            { // parity block relocation
                block_id = final_block_id;
                source_node_id = stripe_group.parity_comp_nodes[final_block_id - code.k_f];
            }
            dst_node_id = stripe_group.post_stripe->indices[final_block_id];

            if (source_node_id == dst_node_id)
            {
                continue;
            }

            // 3.1 read tasks
            TransTask *read_task = new TransTask(TransTaskType::READ_RELOC_BLK, stripe_group.id, final_block_id, stripe_id_global, stripe_id, block_id, source_node_id, source_node_id);

            sg_tasks[stripe_group.id].push_back(read_task);
            sg_read_tasks[stripe_group.id].push_back(read_task);

            // 3.2 transfer tasks
            TransTask *transfer_task = new TransTask(TransTaskType::TRANSFER_RELOC_BLK, stripe_group.id, final_block_id, stripe_id_global, stripe_id, block_id, source_node_id, dst_node_id);
            sg_tasks[stripe_group.id].push_back(transfer_task);
            sg_transfer_tasks[stripe_group.id].push_back(transfer_task);

            // 3.3 write tasks
            TransTask *write_task = new TransTask(TransTaskType::WRITE_BLK, stripe_group.id, final_block_id, stripe_id_global, stripe_id, block_id, dst_node_id, dst_node_id);
            sg_tasks[stripe_group.id].push_back(write_task);
            sg_write_tasks[stripe_group.id].push_back(write_task);

            // 3.4 delete tasks
            TransTask *delete_task = new TransTask(TransTaskType::DELETE_BLK, stripe_group.id, final_block_id, stripe_id_global, stripe_id, block_id, source_node_id, source_node_id);
            sg_tasks[stripe_group.id].push_back(delete_task);
            sg_delete_tasks[stripe_group.id].push_back(delete_task);
        }
    }
}
