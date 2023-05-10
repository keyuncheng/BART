#include "StripeMerge.hh"

StripeMerge::StripeMerge(mt19937 &_random_generator) : random_generator(_random_generator)
{
}

StripeMerge::~StripeMerge()
{
}

void StripeMerge::genTransSolution(StripeBatch &stripe_batch, TransSolution &trans_solution)
{
    // check whether the code is valid for SM
    if (stripe_batch.code.isValidForPM() == false)
    {
        printf("invalid parameters for StripeMerge\n");
        return;
    }

    // Step 1: enumerate all possible stripe groups; pick non-overlapped stripe groups in ascending order of transition costs (bandwidth)
    printf("Step 1: construct stripe groups\n");
    stripe_batch.constructSGByCost();
    // stripe_batch.constructSGInSequence();
    // stripe_batch.constructSGByRandomPick();
    stripe_batch.print();

    // Step 2: generate transition solutions from all stripe groups
    printf("Step 2: generate transition solution\n");
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;
        genTransSolution(stripe_group, trans_solution);
    }
}

void StripeMerge::genTransSolution(StripeGroup &stripe_group, TransSolution &trans_solution)
{

    ConvertibleCode &code = stripe_group.code;
    uint16_t num_nodes = stripe_group.settings.num_nodes;

    u16string final_block_dist = stripe_group.data_dist;                                                                         // record block distribution in final stripe (initialize with data blocks first)
    vector<uint16_t> final_block_placement(code.n_f, INVALID_NODE_ID);                                                           // record current block placement in final stripe
    vector<pair<uint8_t, uint8_t>> final_data_block_source(code.k_f, pair<uint8_t, uint8_t>(INVALID_STRIPE_ID, INVALID_BLK_ID)); // record source of data block (stripe_id, block_id) from the initial stripes in the final stripes

    // record current data block placement
    uint8_t final_block_id = 0;
    for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
    {
        for (uint8_t block_id = 0; block_id < code.k_i; block_id++)
        {
            final_block_placement[final_block_id] = stripe_group.sg_stripes[stripe_id]->indices[block_id];
            final_data_block_source[final_block_id].first = stripe_id;
            final_data_block_source[final_block_id].second = block_id;
            final_block_id++;
        }
    }

    // Step 2: parity generation

    // for each parity block, find the node with minimum bandwidth for parity generation
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        // candidate nodes for parity merging
        u16string &parity_dist = stripe_group.parity_dists[parity_id];

        uint16_t min_bw_node = UINT16_MAX;
        uint8_t min_bw = UINT8_MAX;

        for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
        {
            // parity generation bw + parity relocation bw (check if the node already stores a block)
            uint8_t pm_bw = (code.alpha - parity_dist[node_id]) + ((final_block_dist[node_id] > 0) == true ? 1 : 0);

            if (pm_bw < min_bw)
            {
                min_bw = pm_bw;
                min_bw_node = node_id;

                // no need to search if min_bw = 0
                if (min_bw == 0)
                {
                    break;
                }
            }
        }

        final_block_dist[min_bw_node]++;                           // the block is computed at min_bw_node, which may need to be relocated later
        final_block_placement[code.k_f + parity_id] = min_bw_node; // mark the node for parity block computation

        for (size_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
        {
            Stripe *stripe = stripe_group.sg_stripes[stripe_id];
            uint16_t parity_node_id = stripe->indices[code.k_i + parity_id];

            if (TRANSFER_TASKS_ONLY == false)
            {
                // 2.1 create parity block read tasks
                TransTask *read_task = new TransTask(TransTaskType::READ_BLK, stripe_group.id, stripe->id, stripe_id, code.k_i + parity_id, parity_node_id);
                trans_solution.sg_tasks[stripe_group.id].push_back(read_task);
                trans_solution.sg_read_tasks[stripe_group.id].push_back(read_task);
            }

            // 2.2 create transfer task
            // check if the parity block is not located at min_bw_node
            if (parity_node_id != min_bw_node)
            {
                TransTask *transfer_task = new TransTask(TransTaskType::TRANSFER_BLK, stripe_group.id, stripe->id, stripe_id, code.k_i + parity_id, parity_node_id);
                transfer_task->dst_node_id = min_bw_node;
                trans_solution.sg_tasks[stripe_group.id].push_back(transfer_task);
                trans_solution.sg_transfer_tasks[stripe_group.id].push_back(transfer_task);
            }
        }

        if (TRANSFER_TASKS_ONLY == false)
        {
            // 2.3 create compute task
            TransTask *compute_task = new TransTask(TransTaskType::COMPUTE_BLK, stripe_group.id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, code.k_f + parity_id, min_bw_node);
            trans_solution.sg_tasks[stripe_group.id].push_back(compute_task);
            trans_solution.sg_compute_tasks[stripe_group.id].push_back(compute_task);

            // 2.4 create write task
            TransTask *write_task = new TransTask(TransTaskType::WRITE_BLK, stripe_group.id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, code.k_f + parity_id, min_bw_node);
            trans_solution.sg_tasks[stripe_group.id].push_back(write_task);
            trans_solution.sg_write_tasks[stripe_group.id].push_back(write_task);
        }

        if (TRANSFER_TASKS_ONLY == false)
        {
            for (size_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
            {
                Stripe *stripe = stripe_group.sg_stripes[stripe_id];
                uint16_t parity_node_id = stripe->indices[code.k_i + parity_id];

                // 2.5 create parity block delete tasks
                TransTask *delete_task = new TransTask(TransTaskType::DELETE_BLK, stripe_group.id, stripe->id, stripe_id, code.k_f + parity_id, parity_node_id);
                trans_solution.sg_tasks[stripe_group.id].push_back(delete_task);
                trans_solution.sg_delete_tasks[stripe_group.id].push_back(delete_task);
            }
        }
    }

    // Step 3. create (data and parity) block relocation tasks

    // find blocks that needs relocation
    vector<bool> is_node_placed(num_nodes, false);
    vector<uint8_t> block_to_reloc;
    for (uint8_t final_block_id = 0; final_block_id < code.n_f; final_block_id++)
    {
        uint16_t cur_placed_node_id = final_block_placement[final_block_id];
        if (is_node_placed[cur_placed_node_id] == false)
        {
            is_node_placed[cur_placed_node_id] = true;
        }
        else
        {
            block_to_reloc.push_back(final_block_id);
        }
    }

    vector<uint16_t> available_reloc_nodes; // available nodes for block relocation

    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        if (is_node_placed[node_id] == false)
        {
            // add node as candidate nodes for relocation
            available_reloc_nodes.push_back(node_id);
        }
    }

    // printf("final_block_placement: \n");
    // Utils::printVector(final_block_placement);
    // printf("block_to_reloc: \n");
    // Utils::printVector(block_to_reloc);
    // printf("available_reloc_nodes: \n");
    // Utils::printVector(available_reloc_nodes);

    // shuffle the available nodes and store in the first <block_to_reloc> nodes
    shuffle(available_reloc_nodes.begin(), available_reloc_nodes.end(), random_generator);

    // create block transfer tasks
    for (uint8_t idx = 0; idx < block_to_reloc.size(); idx++)
    {
        uint8_t final_block_id = block_to_reloc[idx];
        uint16_t source_node = final_block_placement[final_block_id];
        uint16_t dst_node = available_reloc_nodes[idx];

        uint32_t stripe_id_global = INVALID_STRIPE_ID_GLOBAL;
        uint8_t stripe_id = INVALID_STRIPE_ID;
        uint8_t block_id = INVALID_BLK_ID;
        if (final_block_id < code.k_f)
        {
            stripe_id = final_data_block_source[final_block_id].first;
            block_id = final_data_block_source[final_block_id].second;
            stripe_id_global = stripe_group.sg_stripes[stripe_id]->id;
        }

        if (TRANSFER_TASKS_ONLY == false)
        {
            // 3.1 read tasks
            TransTask *read_task = new TransTask(TransTaskType::READ_BLK, stripe_group.id, stripe_id_global, stripe_id, block_id, source_node);

            trans_solution.sg_tasks[stripe_group.id].push_back(read_task);
            trans_solution.sg_read_tasks[stripe_group.id].push_back(read_task);
        }

        // 3.2 transfer tasks
        TransTask *transfer_task = new TransTask(TransTaskType::TRANSFER_BLK, stripe_group.id, stripe_id_global, stripe_id, block_id, source_node);
        transfer_task->dst_node_id = dst_node;
        trans_solution.sg_tasks[stripe_group.id].push_back(transfer_task);
        trans_solution.sg_transfer_tasks[stripe_group.id].push_back(transfer_task);

        if (TRANSFER_TASKS_ONLY == false)
        {
            // 3.3 write tasks
            TransTask *write_task = new TransTask(TransTaskType::WRITE_BLK, stripe_group.id, stripe_id_global, stripe_id, block_id, dst_node);
            trans_solution.sg_tasks[stripe_group.id].push_back(write_task);
            trans_solution.sg_write_tasks[stripe_group.id].push_back(write_task);

            // 3.4 delete tasks
            TransTask *delete_task = new TransTask(TransTaskType::DELETE_BLK, stripe_group.id, stripe_group.sg_stripes[stripe_id]->id, stripe_id, block_id, source_node);
            trans_solution.sg_tasks[stripe_group.id].push_back(delete_task);
            trans_solution.sg_delete_tasks[stripe_group.id].push_back(delete_task);
        }
    }
}