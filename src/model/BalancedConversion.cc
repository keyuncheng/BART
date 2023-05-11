#include "BalancedConversion.hh"

BalancedConversion::BalancedConversion(mt19937 &_random_generator) : random_generator(_random_generator)
{
}

BalancedConversion::~BalancedConversion()
{
}

void BalancedConversion::genTransSolution(StripeBatch &stripe_batch, TransSolution &trans_solution)
{
    // Step 1: construct stripe groups
    printf("Step 1: construct stripe groups\n");
    stripe_batch.constructSGByCost();
    // stripe_batch.constructSGInSequence();
    // stripe_batch.constructSGByRandomPick();
    stripe_batch.print();

    // Step 2 - 3: select encoding method and parity generation nodes
    printf("Step 2 - 3: select encoding method and parity generation nodes\n");
    genParityGenerationLTs(stripe_batch);

    // Step 4: scheduling (data and parity) block relocation
    printf("Step 4: scheduling (data and parity) block relocation\n");
    genBlockRelocation(stripe_batch, trans_solution);
}

void BalancedConversion::genParityGenerationLTs(StripeBatch &stripe_batch)
{
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    // maintain a dynamic load table for current stripe groups
    LoadTable cur_lt;
    cur_lt.slt.assign(num_nodes, 0);
    cur_lt.rlt.assign(num_nodes, 0);

    // first, calculate the send load distribution for data relocation across all stripe groups, as given the data placement for each stripe group, the send load distribution is deterministic; at this point, the receive load for data relocation is not determined and can be scheduled
    for (auto &item : stripe_batch.selected_sgs)
    {
        u16string &data_dist = item.second.data_dist;
        for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
        {
            if (data_dist[node_id] > 1)
            {
                cur_lt.slt[node_id] += data_dist[node_id] - 1;
            }
        }
    }
    cur_lt.bw = accumulate(cur_lt.slt.begin(), cur_lt.slt.end(), 0);

    printf("current_lt initialized with data relocation send load:\n");
    printf("send load: ");
    Utils::printVector(cur_lt.slt);
    printf("recv load: ");
    Utils::printVector(cur_lt.rlt);
    printf("bandwidth : %u\n", cur_lt.bw);

    // obtain candidate load tables by enumerating encoding methods and nodes for parity generation
    unordered_map<uint32_t, bool> is_sg_perfect_pm;
    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        // before the enumeration, check the bandwidth for parity computation; if parity generation satisfy perfect parity merging (bw = 0), then directly apply the scheme
        if (stripe_group.getMinPMBW() == 0)
        {
            // generate partial load table for parity merging (with send load for data relocation only)
            stripe_group.genParityGenScheme4PerfectPM();
            is_sg_perfect_pm[sg_id] = true;
            continue;
        }

        // for other groups with non-zero parity generation bandwidth, generate all load tables
        vector<LoadTable> partial_lts;
        stripe_group.genAllPartialLTs4ParityGen();
    }

    // use a while loop until the solution cannot be further optimized (i.e., cannot further improve load balance and then bandwidth)
    // Q: Do we need to set a max_iter to avoid too much iteration?
    uint64_t iter = 0;
    uint32_t cur_max_load_iter = UINT32_MAX;
    uint32_t cur_bw_iter = UINT32_MAX;
    while (true)
    {
        iter++;
        printf("iteration: %ld\n", iter);
        for (auto &item : stripe_batch.selected_sgs)
        {
            uint32_t sg_id = item.first;
            StripeGroup &stripe_group = item.second;

            // skip perfect merging stripe groups
            if (is_sg_perfect_pm[sg_id] == true)
            {
                continue;
            }

            // based on the send load from data relocation, for each stripe group, generate all possible load tables (for re-encoding and parity merging) and finds the most load-balanced load table, which determines the parity generation method and node

            // metric for choosing load tables: (1) minimize max load: max(max(cur_lt.slt + slt), max(cur_lt.rlt + rlt)); if (1) finds multiple load tables, choose the load table with minimum bandwidth; if (2) also finds multiple lts, randomly pick one

            uint32_t min_max_load = UINT32_MAX;
            uint32_t min_bw = UINT8_MAX;
            vector<LoadTable *> best_lts;
            for (auto &cand_lt : stripe_group.cand_partial_lts)
            {
                // calculate cur_lt after applying the cand_lt
                LoadTable cur_lt_after = cur_lt;
                Utils::dotAddVectors(cur_lt_after.slt, cand_lt.slt, cur_lt_after.slt);
                Utils::dotAddVectors(cur_lt_after.rlt, cand_lt.rlt, cur_lt_after.rlt);

                // if we already find and applied a load table for the stripe group, need to subtract it
                if (stripe_group.applied_lt.approach != EncodeMethod::UNKNOWN_METHOD)
                {
                    Utils::dotSubVectors(cur_lt_after.slt, stripe_group.applied_lt.slt, cur_lt_after.slt);
                    Utils::dotSubVectors(cur_lt_after.rlt, stripe_group.applied_lt.rlt, cur_lt_after.rlt);
                }

                uint32_t max_load = max(*max_element(cur_lt_after.slt.begin(), cur_lt_after.slt.end()), *max_element(cur_lt_after.rlt.begin(), cur_lt_after.rlt.end()));

                // update min_max_load and min_bw
                if (max_load < min_max_load || (max_load == min_max_load && cand_lt.bw < min_bw))
                {
                    min_max_load = max_load;
                    min_bw = cand_lt.bw;
                    best_lts.clear();
                    best_lts.push_back(&cand_lt);
                }
                else if (max_load == min_max_load && cand_lt.bw == min_bw)
                {
                    best_lts.push_back(&cand_lt);
                }
            }

            // randomly choose a best load table
            uint random_pos = Utils::randomUInt(0, best_lts.size() - 1, random_generator);
            LoadTable *selected_lt = best_lts[random_pos];

            // update current load table
            if (stripe_group.applied_lt.approach != EncodeMethod::UNKNOWN_METHOD)
            {
                // if we already find and applied a load table for the stripe group, need to subtract it
                Utils::dotSubVectors(cur_lt.slt, stripe_group.applied_lt.slt, cur_lt.slt);
                Utils::dotSubVectors(cur_lt.rlt, stripe_group.applied_lt.rlt, cur_lt.rlt);
                cur_lt.bw -= stripe_group.applied_lt.bw;
            }

            Utils::dotAddVectors(cur_lt.slt, selected_lt->slt, cur_lt.slt);
            Utils::dotAddVectors(cur_lt.rlt, selected_lt->rlt, cur_lt.rlt);
            cur_lt.bw += selected_lt->bw;

            // apply the load table for stripe group
            stripe_group.applied_lt = *selected_lt;

            // printf("applied_lt for stripe group %u:\n", stripe_group.id);
            // printf("send load: ");
            // Utils::printVector(stripe_group.applied_lt.slt);
            // printf("recv load: ");
            // Utils::printVector(stripe_group.applied_lt.rlt);
            // printf("encode method: %u, bandwidth : %u\n", stripe_group.applied_lt.approach, stripe_group.applied_lt.bw);

            // printf("current_lt after choosing stripe group %u:\n", stripe_group.id);
            // printf("send load: ");
            // Utils::printVector(cur_lt.slt);
            // printf("recv load: ");
            // Utils::printVector(cur_lt.rlt);
            // printf("bandwidth : %u\n", cur_lt.bw);
        }
        // summarize the current bw
        uint32_t cur_max_load = *max_element(cur_lt.slt.begin(), cur_lt.slt.end());
        bool improved = cur_max_load < cur_max_load_iter || (cur_max_load == cur_max_load_iter && cur_lt.bw < cur_bw_iter);
        if (improved == true)
        {
            // update the max load and bw
            cur_max_load_iter = cur_max_load;
            cur_bw_iter = cur_lt.bw;
        }
        else
        {
            // the solution cannot be further optimized
            break;
        }
        printf("cur_lt after iteration %lu:\n", iter);
        printf("send load: ");
        Utils::printVector(cur_lt.slt);
        printf("recv load: ");
        Utils::printVector(cur_lt.rlt);
        printf("bandwidth: %u\n", cur_lt.bw);
    }

    // clear the candidate load tables for memory saving
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;
        stripe_group.cand_partial_lts.clear();
    }

    uint32_t num_re_groups = 0;
    for (auto &item : stripe_batch.selected_sgs)
    {
        num_re_groups += (item.second.applied_lt.approach == EncodeMethod::RE_ENCODE ? 1 : 0);
    }

    printf("final load table with data relocation (send load only), parity generation (both send and receive load) and parity relocation (send load only):\n");
    printf("send load: ");
    Utils::printVector(cur_lt.slt);
    printf("recv load: ");
    Utils::printVector(cur_lt.rlt);
    printf("bandwidth: %u\n", cur_lt.bw);
    printf("number of re-encoding groups: (%u / %lu), num of parity merging groups: (%lu / %lu)\n", num_re_groups, stripe_batch.selected_sgs.size(), (stripe_batch.selected_sgs.size() - num_re_groups), stripe_batch.selected_sgs.size());
}

void BalancedConversion::genBlockRelocation(StripeBatch &stripe_batch, TransSolution &trans_solution)
{
    // NOTE: at this point, the parity generation (encoding methods and nodes are fixed); in this step we schedule the nodes to relocate the data and parity blocks for each stripe group

    ConvertibleCode &code = stripe_batch.code;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    // step 1: for each stripe group, find the data and parity blocks that needs to be relocated, and available nodes for relocation
    unordered_map<uint32_t, vector<uint16_t>> sg_final_block_placement;
    unordered_map<uint32_t, vector<pair<uint8_t, uint16_t>>> sg_blocks_to_reloc; // for each block in the final stripe, record <final_block_id, node it stored>
    unordered_map<uint32_t, vector<uint16_t>> sg_avail_nodes;

    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        // init block placement
        sg_final_block_placement[sg_id] = vector<uint16_t>(code.n_f, INVALID_NODE_ID);
        vector<bool> is_node_relocated(num_nodes, 0);

        // find the blocks to be relocated
        for (uint8_t final_block_id = 0; final_block_id < code.n_f; final_block_id++)
        {
            uint16_t cur_placed_node_id = INVALID_NODE_ID;
            if (final_block_id < code.k_f)
            { // data block
                uint8_t stripe_id = final_block_id / code.lambda_i;
                uint8_t block_id = final_block_id % code.lambda_i;
                cur_placed_node_id = stripe_group.sg_stripes[stripe_id]->indices[block_id];
            }
            else
            { // parity block
                uint8_t parity_id = final_block_id - code.k_f;
                cur_placed_node_id = stripe_group.applied_lt.enc_nodes[parity_id]; // check the applied load table
            }
            if (is_node_relocated[cur_placed_node_id] == false)
            {
                // directly store the block at the node
                sg_final_block_placement[sg_id][final_block_id] = cur_placed_node_id;
                is_node_relocated[cur_placed_node_id] = true; // mark the node as relocated
            }
            else
            {
                // mark the block to be relocated
                sg_blocks_to_reloc[sg_id].push_back(pair<uint8_t, uint16_t>(final_block_id, cur_placed_node_id));
            }
        }

        // find available nodes
        for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
        {
            if (is_node_relocated[node_id] == false)
            {
                sg_avail_nodes[sg_id].push_back(node_id);
            }
        }
    }

    // step 2: construct bipartite graph
    Bipartite bipartite;

    unordered_map<uint32_t, uint64_t> node2vtx_map;               // node to vertex map
    unordered_map<uint64_t, uint32_t> vtx2node_map;               // node to vertex map
    unordered_map<uint64_t, pair<uint32_t, uint8_t>> lvtx2sg_map; // left vertex to stripe group map
    unordered_map<uint32_t, vector<uint64_t>> sg2lvtx_map;        // stripe group to left vertex map (corresponding to sg_blocks_to_reloc)

    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        // for each node, create a right vertex
        uint64_t rvtx_id = bipartite.addVertex(VertexType::RIGHT);
        node2vtx_map[node_id] = rvtx_id;
        vtx2node_map[rvtx_id] = node_id;
    }

    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;

        // for each block to relocate, create a left vertex
        for (auto &item : sg_blocks_to_reloc[sg_id])
        {
            uint64_t lvtx_id = bipartite.addVertex(VertexType::LEFT);
            lvtx2sg_map[lvtx_id] = pair<uint32_t, uint8_t>(sg_id, item.first); // record stripe group information for the vertex
            sg2lvtx_map[sg_id].push_back(lvtx_id);

            // for each available nodes of the stripe group, add an edge
            for (auto avail_node_id : sg_avail_nodes[sg_id])
            {
                bipartite.addEdge(bipartite.left_vertices_map[lvtx_id], bipartite.right_vertices_map[node2vtx_map[avail_node_id]]);
            }
        }
    }

    // accumulate receive load of parity generation
    LoadTable lt;
    lt.rlt.assign(num_nodes, 0);
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;
        if (stripe_group.applied_lt.approach == EncodeMethod::RE_ENCODE)
        {
            uint16_t enc_node = stripe_group.applied_lt.enc_nodes[0];
            lt.rlt[enc_node] += code.k_f - stripe_group.data_dist[enc_node]; // require to receive # of data blocks
        }
        else if (stripe_group.applied_lt.approach == EncodeMethod::PARITY_MERGE)
        {
            for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
            {
                uint16_t enc_node = stripe_group.applied_lt.enc_nodes[parity_id];
                lt.rlt[enc_node] += code.lambda_i - stripe_group.parity_dists[parity_id][enc_node]; // require to receive # of parity blocks
            }
        }
    }

    // initialize bipartite right vertices degrees with the receive load of parity generation
    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        Vertex &rvtx = *bipartite.right_vertices_map[node2vtx_map[node_id]];
        rvtx.in_degree = lt.rlt[node_id];
    }

    // step 3: find optimal semi-matching (based on the initial receive load)
    vector<uint64_t> sm_edges = bipartite.findOptSemiMatching();

    // update the final_block_placement from chosen edges
    for (auto edge_id : sm_edges)
    {
        Edge &edge = bipartite.edges_map[edge_id];
        Vertex &lvtx = *bipartite.left_vertices_map[edge.lvtx->id];
        uint32_t sg_id = lvtx2sg_map[lvtx.id].first;
        uint8_t final_block_id = lvtx2sg_map[lvtx.id].second;
        uint16_t reloc_node_id = vtx2node_map[edge.rvtx->id];

        sg_final_block_placement[sg_id][final_block_id] = reloc_node_id; // record place node_id
    }

    // step 4: build solution
    // buildTransTasks(stripe_batch, sg_blocks_to_reloc, sg_final_block_placement, trans_solution);
}

void BalancedConversion::buildTransTasks(StripeBatch &stripe_batch, unordered_map<uint32_t, vector<pair<uint8_t, uint16_t>>> sg_blocks_to_reloc, unordered_map<uint32_t, vector<uint16_t>> sg_final_block_placement, TransSolution &trans_solution)
{
    ConvertibleCode &code = stripe_batch.code;
    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        // 1 - 2: generate tasks for parity computation

        // 1. Re-encoding
        if (stripe_group.applied_lt.approach == EncodeMethod::RE_ENCODE)
        {
            uint16_t enc_node = stripe_group.applied_lt.enc_nodes[0];

            // read code.k_f data blocks
            for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
            {
                Stripe *stripe = stripe_group.sg_stripes[stripe_id];
                for (uint8_t data_block_id = 0; data_block_id < code.k_i; data_block_id++)
                {
                    uint16_t data_node_id = stripe->indices[data_block_id];

                    if (TRANSFER_TASKS_ONLY == false)
                    {
                        // 1.1 create data block read tasks
                        TransTask *read_task = new TransTask(TransTaskType::READ_BLK, stripe_group.id, stripe->id, stripe_id, data_block_id, data_node_id);
                        trans_solution.sg_tasks[stripe_group.id].push_back(read_task);
                        trans_solution.sg_read_tasks[stripe_group.id].push_back(read_task);
                    }

                    // 1.2 create transfer task
                    // check if the parity block is not located at min_bw_node
                    if (data_node_id != enc_node)
                    {
                        TransTask *transfer_task = new TransTask(TransTaskType::TRANSFER_BLK, stripe_group.id, stripe->id, stripe_id, data_block_id, data_node_id);
                        transfer_task->dst_node_id = enc_node;
                        trans_solution.sg_tasks[stripe_group.id].push_back(transfer_task);
                        trans_solution.sg_transfer_tasks[stripe_group.id].push_back(transfer_task);
                    }
                }

                if (TRANSFER_TASKS_ONLY == false)
                {
                    // 1.3 create compute task
                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        TransTask *compute_task = new TransTask(TransTaskType::COMPUTE_BLK, stripe_group.id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, code.k_f + parity_id, enc_node);
                        trans_solution.sg_tasks[stripe_group.id].push_back(compute_task);
                        trans_solution.sg_compute_tasks[stripe_group.id].push_back(compute_task);

                        // 1.4 create write task
                        TransTask *write_task = new TransTask(TransTaskType::WRITE_BLK, stripe_group.id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, code.k_f + parity_id, enc_node);
                        trans_solution.sg_tasks[stripe_group.id].push_back(write_task);
                        trans_solution.sg_write_tasks[stripe_group.id].push_back(write_task);
                    }
                }
            }
        }
        else if (stripe_group.applied_lt.approach == EncodeMethod::PARITY_MERGE)
        {
            // 2. Parity Merging
            for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
            {
                uint16_t enc_node = stripe_group.applied_lt.enc_nodes[parity_id];

                for (uint32_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
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
                    // check if the parity block is not located at enc_node
                    if (parity_node_id != enc_node)
                    {
                        TransTask *transfer_task = new TransTask(TransTaskType::TRANSFER_BLK, stripe_group.id, stripe->id, stripe_id, code.k_i + parity_id, parity_node_id);
                        transfer_task->dst_node_id = enc_node;
                        trans_solution.sg_tasks[stripe_group.id].push_back(transfer_task);
                        trans_solution.sg_transfer_tasks[stripe_group.id].push_back(transfer_task);
                    }
                }

                if (TRANSFER_TASKS_ONLY == false)
                {
                    // 2.3 create compute task
                    TransTask *compute_task = new TransTask(TransTaskType::COMPUTE_BLK, stripe_group.id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, code.k_f + parity_id, enc_node);
                    trans_solution.sg_tasks[stripe_group.id].push_back(compute_task);
                    trans_solution.sg_compute_tasks[stripe_group.id].push_back(compute_task);

                    // 2.4 create write task
                    TransTask *write_task = new TransTask(TransTaskType::WRITE_BLK, stripe_group.id, INVALID_STRIPE_ID, INVALID_STRIPE_ID, code.k_f + parity_id, enc_node);
                    trans_solution.sg_tasks[stripe_group.id].push_back(write_task);
                    trans_solution.sg_write_tasks[stripe_group.id].push_back(write_task);
                }

                if (TRANSFER_TASKS_ONLY == false)
                {
                    for (uint32_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
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
        }

        // 3. generate tasks for (data and parity) block relocation
        vector<pair<uint8_t, uint16_t>> &blocks_to_reloc = sg_blocks_to_reloc[sg_id];
        vector<uint16_t> final_block_placement = sg_final_block_placement[sg_id];

        for (auto &item : blocks_to_reloc)
        {
            uint8_t final_block_id = item.first;
            uint16_t source_node = item.second;
            uint16_t dst_node = final_block_placement[final_block_id];

            uint32_t stripe_id_global = INVALID_STRIPE_ID_GLOBAL;
            uint8_t stripe_id = INVALID_STRIPE_ID;
            uint8_t block_id = INVALID_BLK_ID;
            if (final_block_id < code.k_f)
            {
                stripe_id = final_block_id / code.lambda_i;
                block_id = final_block_id % code.lambda_i;
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
}

// void BalancedConversion::getSolutionForStripeBatchGlobal(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator)
// {
//     ConvertibleCode &code = stripe_batch.getCode();
//     ClusterSettings &settings = stripe_batch.getClusterSettings();

//     vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

//     size_t num_stripe_groups = stripe_groups.size();

//     // initialize solutions
//     solutions.clear();

//     // Step 1: enumeration of approaches
//     vector<vector<size_t>> approach_candidates;
//     if (code.isValidForPM() == false)
//     {
//         // re-encoding only
//         approach_candidates.push_back(vector<size_t>(num_stripe_groups, (size_t)EncodeMethod::RE_ENCODE));
//     }
//     else
//     {
//         // re-encoding only
//         // approach_candidates.push_back(vector<size_t>(num_stripe_groups, (size_t) EncodeMethod::RE_ENCODE));

//         // parity merging only
//         approach_candidates.push_back(vector<size_t>(num_stripe_groups, (size_t)EncodeMethod::PARITY_MERGE));

//         // // // re-encoding + parity merging (generate 2^num_sg enumerations)
//         // approach_candidates = Utils::getPermutations(num_stripe_groups, 2);
//     }

//     size_t num_approach_candidates = approach_candidates.size();

//     printf("num_stripe_groups: %ld, number of approach candidates: %ld\n", num_stripe_groups, num_approach_candidates);

//     // for (size_t app_cand_id = 0; app_cand_id < num_approach_candidates; app_cand_id++) {
//     //     Utils::printUIntVector(approach_candidates[app_cand_id]);
//     // }

//     // Step 2: construct model for each approach candidate, and find the one with minimum max-load
//     size_t best_app_cand_id = INVALID_ID;
//     size_t best_max_load = SIZE_MAX;
//     vector<vector<size_t>> best_solutions;

//     for (size_t app_cand_id = 0; app_cand_id < num_approach_candidates; app_cand_id++)
//     {
//         vector<size_t> approach_candidate = approach_candidates[app_cand_id];

//         printf("approach %ld:\n", app_cand_id);
//         Utils::printUIntVector(approach_candidates[app_cand_id]);

//         // Step 2.1: construct recv bipartite graph
//         RecvBipartite recv_bipartite;
//         recv_bipartite.constructStripeBatchWithApproaches(stripe_batch, approach_candidate);

//         // recv_bipartite.print_block_metastore();
//         // recv_bipartite.print();

//         // find edges
//         vector<size_t> sol_edges;
//         // recv_bipartite.findEdgesWithApproachesGreedy(stripe_batch, sol_edges, random_generator);

//         // recv_bipartite.findEdgesWithApproachesGreedySorted(stripe_batch, sol_edges, random_generator);
//         recv_bipartite.findEdgesWithApproachesGreedySortedSAR(stripe_batch, sol_edges, random_generator);

//         // construct partial solutions from recv graph
//         vector<vector<size_t>> partial_solutions, solutions;
//         recv_bipartite.constructPartialSolutionFromEdges(stripe_batch, sol_edges, partial_solutions);
//         recv_bipartite.updatePartialSolutionFromRecvGraph(stripe_batch, partial_solutions, solutions);

//         vector<size_t> send_load_dist, recv_load_dist;
//         Utils::getLoadDist(code, settings, solutions, send_load_dist, recv_load_dist);

//         // get bandwidth
//         size_t bw_bt = 0;
//         for (auto item : send_load_dist)
//         {
//             bw_bt += item;
//         }

//         size_t min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
//         size_t max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
//         size_t min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
//         size_t max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());

//         printf("Balanced Transition (approach %ld) send load distribution:, minimum_load: %ld, maximum_load: %ld\n", app_cand_id,
//                min_send_load, max_send_load);
//         Utils::printUIntVector(send_load_dist);
//         printf("Balanced Transition (approach %ld) recv load distribution:, minimum_load: %ld, maximum_load: %ld\n", app_cand_id,
//                min_recv_load, max_recv_load);
//         Utils::printUIntVector(recv_load_dist);
//         printf("Balanced Transition (approach %ld) bandwidth: %ld\n", app_cand_id, bw_bt);

//         size_t cand_max_load = max(max_send_load, max_recv_load);
//         if (cand_max_load < best_max_load)
//         {
//             best_max_load = cand_max_load;
//             best_app_cand_id = app_cand_id;
//             best_solutions = solutions;
//         }
//     }

//     if (best_app_cand_id != INVALID_ID)
//     {
//         printf("found best candidate id: %ld, best_max_load: %ld, approaches:\n", best_app_cand_id, best_max_load);
//         Utils::printUIntVector(approach_candidates[best_app_cand_id]);
//         solutions = best_solutions;
//     }

//     // put it into the solution for the batch
//     printf("solutions for stripe_batch (size: %ld): \n", solutions.size());
//     for (auto solution : solutions)
//     {
//         printf("stripe %ld, block %ld, from: %ld, to: %ld\n", solution[0], solution[1], solution[2], solution[3]);
//     }
// }

// void BalancedConversion::getSolutionForStripeBatchGreedy(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator)
// {
//     ConvertibleCode &code = stripe_batch.getCode();
//     ClusterSettings &settings = stripe_batch.getClusterSettings();

//     vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

//     size_t num_stripe_groups = stripe_groups.size();

//     // initialize solutions
//     solutions.clear();

//     StripeBatch cur_stripe_batch = stripe_batch;
//     vector<StripeGroup> &cur_stripe_group = cur_stripe_batch.getStripeGroups();
//     cur_stripe_group.clear();
//     vector<size_t> cur_approaches;
//     vector<vector<size_t>> cur_solutions;

//     // Step 1: for each stripe group, greedily finds the most load-balanced approach
//     for (size_t sg_id = 0; sg_id < num_stripe_groups; sg_id++)
//     {
//         // add group sg_id to current stripe batch
//         cur_stripe_group.push_back(stripe_groups[sg_id]);

//         // Step 2: construct model for each approach candidate, and find the one with minimum max-load
//         size_t best_approach_id = INVALID_ID;
//         size_t best_max_load = SIZE_MAX;
//         vector<vector<size_t>> best_solutions;

//         for (size_t cand_approach_id = 0; cand_approach_id < 2; cand_approach_id++)
//         {
//             vector<size_t> cand_approaches = cur_approaches;
//             cand_approaches.push_back(cand_approach_id);

//             // construct recv bipartite graph
//             RecvBipartite cur_recv_bipartite;
//             cur_recv_bipartite.constructStripeBatchWithApproaches(cur_stripe_batch, cand_approaches);

//             // find edges
//             vector<size_t> cand_sol_edges;
//             cur_recv_bipartite.findEdgesWithApproachesGreedy(cur_stripe_batch, cand_sol_edges, random_generator);

//             // cur_recv_bipartite.findEdgesWithApproachesGreedySorted(cur_stripe_batch, cand_sol_edges, random_generator);

//             // construct partial solutions from recv graph
//             vector<vector<size_t>> cand_partial_solutions, cand_solutions;
//             cur_recv_bipartite.constructPartialSolutionFromEdges(cur_stripe_batch, cand_sol_edges, cand_partial_solutions);
//             cur_recv_bipartite.updatePartialSolutionFromRecvGraph(cur_stripe_batch, cand_partial_solutions, cand_solutions);

//             vector<size_t> send_load_dist, recv_load_dist;
//             Utils::getLoadDist(code, settings, cand_solutions, send_load_dist, recv_load_dist);

//             // size_t min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
//             size_t max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
//             // size_t min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
//             size_t max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());

//             size_t cand_max_load = max(max_send_load, max_recv_load);
//             if (cand_max_load < best_max_load)
//             {
//                 best_max_load = cand_max_load;
//                 best_approach_id = cand_approach_id;
//                 best_solutions = cand_solutions;
//             }
//         }

//         if (best_approach_id != INVALID_ID)
//         {
//             printf("stripe_group %ld, best approach: %ld, best_max_load: %ld, approaches:\n", sg_id, best_approach_id, best_max_load);
//             cur_approaches.push_back(best_approach_id);
//             cur_solutions = best_solutions;
//             Utils::printUIntVector(cur_approaches);
//         }
//     }
//     // update the best solution
//     solutions = cur_solutions;

//     // // put it into the solution for the batch
//     // printf("solutions for stripe_batch (size: %ld): \n", solutions.size());
//     // for (auto solution : solutions) {
//     //     printf("stripe %ld, block %ld, from: %ld,proachesGreedySorted(stripe_batch, cand_sol_edges, ran to: %ld\n", solution[0], solution[1], solution[2], solution[3]);
//     // }
// }

// void BalancedConversion::getSolutionForStripeBatchIter(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator)
// {
//     ConvertibleCode &code = stripe_batch.getCode();
//     ClusterSettings &settings = stripe_batch.getClusterSettings();

//     vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

//     size_t num_stripe_groups = stripe_groups.size();

//     // initialize solutions
//     solutions.clear();

//     // best approaches (start from all parity merging)
//     vector<size_t> best_approaches(num_stripe_groups, (size_t)EncodeMethod::PARITY_MERGE);
//     vector<vector<size_t>> best_solutions;
//     size_t best_max_load = SIZE_MAX;

//     for (size_t iter = 0; iter < num_stripe_groups; iter++)
//     {
//         printf("iter %ld\n", iter);
//         // Step 2: construct model for each approach candidate, and find the one with minimum max-load
//         // Step 1: for each stripe group, find the best approach
//         for (size_t sg_id = 0; sg_id < num_stripe_groups; sg_id++)
//         {
//             // loop over all approaches
//             for (size_t cand_approach_id = 0; cand_approach_id < 2; cand_approach_id++)
//             {

//                 // if (best_approaches[sg_id] == cand_approach_id) {
//                 //     continue;
//                 // }

//                 vector<size_t> cand_approaches = best_approaches;

//                 // replace sg with a new candidate approach
//                 cand_approaches[sg_id] = cand_approach_id;

//                 // construct recv bipartite graph
//                 RecvBipartite cur_recv_bipartite;
//                 cur_recv_bipartite.constructStripeBatchWithApproaches(stripe_batch, cand_approaches);

//                 // find edges
//                 vector<size_t> cand_sol_edges;
//                 // cur_recv_bipartite.findEdgesWithApproachesGreedy(stripe_batch, cand_sol_edges, random_generator);

//                 cur_recv_bipartite.findEdgesWithApproachesGreedySorted(stripe_batch, cand_sol_edges, random_generator);

//                 // construct partial solutions from recv graph
//                 vector<vector<size_t>> cand_partial_solutions, cand_solutions;
//                 cur_recv_bipartite.constructPartialSolutionFromEdges(stripe_batch, cand_sol_edges, cand_partial_solutions);
//                 cur_recv_bipartite.updatePartialSolutionFromRecvGraph(stripe_batch, cand_partial_solutions, cand_solutions);

//                 vector<size_t> send_load_dist, recv_load_dist;
//                 Utils::getLoadDist(code, settings, cand_solutions, send_load_dist, recv_load_dist);

//                 // size_t min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
//                 size_t max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
//                 // size_t min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
//                 size_t max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());
//                 size_t cand_max_load = max(max_send_load, max_recv_load);

//                 // check if the current load out-performs previous best load
//                 if (cand_max_load < best_max_load)
//                 {
//                     best_max_load = cand_max_load;
//                     best_approaches = cand_approaches;
//                     best_solutions = cand_solutions;

//                     printf("found better approaches, previous_load: %ld, best_max_load: %ld, approaches:\n", best_max_load, cand_max_load);
//                     Utils::printUIntVector(best_approaches);
//                 }
//             }
//         }

//         // update solutions
//         solutions = best_solutions;
//     }

//     // // put it into the solution for the batch
//     // printf("solutions for stripe_batch (size: %ld): \n", solutions.size());
//     // for (auto solution : solutions) {
//     //     printf("stripe %ld, block %ld, from: %ld, to: %ld\n", solution[0], solution[1], solution[2], solution[3]);
//     // }
// }

// void BalancedConversion::getSolutionForStripeBatchAssigned(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator)
// {

//     // initialize solutions
//     solutions.clear();

//     // for (size_t app_cand_id = 0; app_cand_id < num_approach_candidates; app_cand_id++) {
//     //     Utils::printUIntVector(approach_candidates[app_cand_id]);
//     // }

//     // Step 2: construct model for each approach candidate, and find the one with minimum max-load

//     // Step 2.1: construct recv bipartite graph
//     vector<size_t> approaches;
//     // for (auto &item : stripe_batch.getSGApproaches())
//     // {
//     //     approaches.push_back((size_t)item);
//     // }

//     for (size_t sid = 0; sid < stripe_batch.getStripeGroups().size(); sid++)
//     {
//         approaches.push_back((size_t)EncodeMethod::PARITY_MERGE);
//     }

//     RecvBipartite recv_bipartite;
//     recv_bipartite.constructStripeBatchWithApproaches(stripe_batch, approaches);

//     // recv_bipartite.print_block_metastore();
//     // recv_bipartite.print();

//     // find edges
//     vector<size_t> sol_edges;
//     // recv_bipartite.findEdgesWithApproachesGreedy(stripe_batch, sol_edges, random_generator);

//     // recv_bipartite.findEdgesWithApproachesGreedySorted(stripe_batch, sol_edges, random_generator);
//     recv_bipartite.findEdgesWithApproachesGreedySortedSAR(stripe_batch, sol_edges, random_generator);

//     // construct partial solutions from recv graph
//     vector<vector<size_t>> partial_solutions;
//     recv_bipartite.constructPartialSolutionFromEdges(stripe_batch, sol_edges, partial_solutions);
//     recv_bipartite.updatePartialSolutionFromRecvGraph(stripe_batch, partial_solutions, solutions);

//     // put it into the solution for the batch
//     printf("solutions for stripe_batch (size: %ld): \n", solutions.size());
//     for (auto solution : solutions)
//     {
//         printf("stripe %ld, block %ld, from: %ld, to: %ld\n", solution[0], solution[1], solution[2], solution[3]);
//     }
// }