#include "BalancedConversion.hh"

BalancedConversion::BalancedConversion(mt19937 &_random_generator) : random_generator(_random_generator)
{
}

BalancedConversion::~BalancedConversion()
{
}

void BalancedConversion::genSolution(StripeBatch &stripe_batch, string approach)
{
    // Step 1: construct stripe groups
    printf("Step 1: construct stripe groups\n");
    // stripe_batch.constructSGByBW(approach);
    stripe_batch.constructSGByBWGreedy(approach);
    // stripe_batch.print();

    // Step 2: generate parity computation scheme (parity computation method and nodes)
    printf("Step 2: generate parity computation scheme\n");
    // genParityComputation(stripe_batch, approach);
    // genParityComputationOptimized(stripe_batch, approach);
    genParityComputationGreedyOptimized(stripe_batch, approach);

    // Step 3: schedule (data and parity) block relocation
    printf("Step 3: schedule block relocation\n");
    genBlockRelocation(stripe_batch);
}

void BalancedConversion::genParityComputation(StripeBatch &stripe_batch, string approach)
{
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    // maintain a load table for currently selected stripe groups
    LoadTable cur_lt;
    cur_lt.slt.assign(num_nodes, 0);
    cur_lt.rlt.assign(num_nodes, 0);

    // initialize the slt with data relocation across all stripe groups
    // given the selected stripe groups, the send load distribution for data relocation is deterministic, as we can obtain the data placement and know how many data blocks needs to be relocated
    // Note: in this step, the receive load for data relocation is not determined, and will be scheduled in Step 4
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

    // bw is calculated with data relocation
    cur_lt.bw = accumulate(cur_lt.slt.begin(), cur_lt.slt.end(), 0);

    // printf("current_lt initialized with data relocation send load:\n");
    // printf("send load: ");
    // Utils::printVector(cur_lt.slt);
    // printf("recv load: ");
    // Utils::printVector(cur_lt.rlt);
    // printf("bandwidth: %u\n", cur_lt.bw);

    // obtain candidate load tables for parity generation by enumerating all encoding methods and nodes
    vector<bool> is_sg_perfect_pm(stripe_batch.selected_sgs.size(), false);
    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        if (approach == "BTPM" || approach == "BT")
        {
            // before the enumeration, check the bandwidth for parity computation; if parity generation satisfy perfect parity merging (bw = 0), then directly apply the scheme
            if (stripe_group.isPerfectParityMerging() == true)
            {
                // generate parity computation scheme for parity merging
                stripe_group.genParityComputeScheme4PerfectPM();
                is_sg_perfect_pm[sg_id] = true;
                continue;
            }
        }
    }

    // use a while loop until the solution cannot be further optimized (i.e., cannot further improve load balance and then bandwidth)
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

            // for other stripe groups with non-zero parity computation bandwidth, generate all candidate load tables for parity computation
            stripe_group.genPartialLTs4ParityCompute(approach);

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
            size_t random_pos = Utils::randomUInt(0, best_lts.size() - 1, random_generator);
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

            // free up the memory
            stripe_group.cand_partial_lts.resize(0);
            stripe_group.cand_partial_lts.shrink_to_fit();

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

        // printf("cur_lt after iteration %lu:\n", iter);
        // printf("send load: ");
        // Utils::printVector(cur_lt.slt);
        // printf("recv load: ");
        // Utils::printVector(cur_lt.rlt);
        // printf("bandwidth: %u\n", cur_lt.bw);
    }

    // update the metadata for stripe group
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;
        stripe_group.parity_comp_method = stripe_group.applied_lt.approach;
        stripe_group.parity_comp_nodes = stripe_group.applied_lt.enc_nodes;

        // clear the candidate load tables for memory saving
        stripe_group.cand_partial_lts.clear();
    }

    uint32_t num_re_groups = 0;
    for (auto &item : stripe_batch.selected_sgs)
    {
        num_re_groups += (item.second.applied_lt.approach == EncodeMethod::RE_ENCODE ? 1 : 0);
    }

    // printf("final load table with data relocation (send load only), parity generation (both send and receive load) and parity relocation (send load only):\n");
    // printf("send load: ");
    // Utils::printVector(cur_lt.slt);
    // printf("recv load: ");
    // Utils::printVector(cur_lt.rlt);
    // printf("bandwidth: %u\n", cur_lt.bw);
    printf("number of re-encoding groups: (%u / %lu), number of parity merging groups: (%lu / %lu)\n", num_re_groups, stripe_batch.selected_sgs.size(), (stripe_batch.selected_sgs.size() - num_re_groups), stripe_batch.selected_sgs.size());
}

void BalancedConversion::genParityComputationOptimized(StripeBatch &stripe_batch, string approach)
{
    ConvertibleCode &code = stripe_batch.code;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    double finish_time = 0.0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    // maintain a load table for currently selected stripe groups
    LoadTable cur_lt;
    cur_lt.slt.assign(num_nodes, 0);
    cur_lt.rlt.assign(num_nodes, 0);

    // initialize the slt with data relocation across all stripe groups
    // given the selected stripe groups, the send load distribution for data relocation is deterministic, as we can obtain the data placement and know how many data blocks needs to be relocated
    // Note: in this step, the receive load for data relocation is not determined, and will be scheduled in Step 4
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

    // bw is calculated with data relocation
    cur_lt.bw = accumulate(cur_lt.slt.begin(), cur_lt.slt.end(), 0);

    // printf("current_lt initialized with data relocation send load:\n");
    // printf("send load: ");
    // Utils::printVector(cur_lt.slt);
    // printf("recv load: ");
    // Utils::printVector(cur_lt.rlt);
    // printf("bandwidth: %u\n", cur_lt.bw);

    // check whether the stripe can be perfectly merged by parity merging
    vector<bool> is_sg_perfect_pm(stripe_batch.selected_sgs.size(), false);
    if (approach == "BTPM" || approach == "BT")
    {
        for (auto &item : stripe_batch.selected_sgs)
        {
            uint32_t sg_id = item.first;
            StripeGroup &stripe_group = item.second;

            // before the enumeration, check the bandwidth for parity computation; if parity generation satisfy perfect parity merging (bw = 0), then directly apply the scheme
            if (stripe_group.isPerfectParityMerging() == true)
            {
                // generate parity computation scheme for parity merging
                stripe_group.genParityComputeScheme4PerfectPM();
                is_sg_perfect_pm[sg_id] = true;
                continue;
            }
        }
    }

    // use a while loop until the solution cannot be further optimized (i.e., cannot further improve load balance and then bandwidth)
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

            // printf("iter %lu, checking stripe group: %u\n", iter, sg_id);

            // based on the send load from data relocation, for each stripe group, generate all possible solutions for parity computation (for re-encoding and/or parity merging) and finds the most load-balanced solution (which determines the parity generation method and node)

            // metric for choosing load tables: (1) minimize max load: max(max(cur_lt.slt + slt), max(cur_lt.rlt + rlt)); if (1) finds multiple load tables, choose the load table with minimum bandwidth; if (2) also finds multiple lts, randomly pick one

            // uint32_t max_send_load_cur_lt = *max_element(cur_lt.slt.begin(), cur_lt.slt.end());
            // uint32_t max_recv_load_cur_lt = *max_element(cur_lt.rlt.begin(), cur_lt.rlt.end());
            uint32_t min_max_load = UINT32_MAX;
            uint32_t min_bw = UINT8_MAX;
            vector<LoadTable> best_lts;

            // calculate cur_lt_after (after applying the cand_lt)
            LoadTable cur_lt_after = cur_lt;

            // if we already find and applied a load table for the stripe group, need to subtract it before changing the solution
            if (stripe_group.applied_lt.approach != EncodeMethod::UNKNOWN_METHOD)
            {
                Utils::dotSubVectors(cur_lt_after.slt, stripe_group.applied_lt.slt, cur_lt_after.slt);
                Utils::dotSubVectors(cur_lt_after.rlt, stripe_group.applied_lt.rlt, cur_lt_after.rlt);
            }

            if (approach == "BTRE" || approach == "BT")
            { // re-encoding

                LoadTable cur_lt_after_re_base = cur_lt_after;

                // add data distribution to form the base re-encoding solution
                Utils::dotAddVectors(cur_lt_after_re_base.slt, stripe_group.data_dist, cur_lt_after_re_base.slt);

                // // add all data blocks to form the base re-encoding solution
                // for (auto &stripe : stripe_group.pre_stripes)
                // {
                //     for (uint8_t data_block_id = 0; data_block_id < code.k_i; data_block_id++)
                //     {
                //         uint16_t data_block_placed_node = stripe->indices[data_block_id];
                //         cur_lt_after_re_base.slt[data_block_placed_node]++;
                //     }
                // }

                // maximum send load with base solution for re-encoding
                uint32_t max_send_load_re_base = *max_element(cur_lt_after_re_base.slt.begin(), cur_lt_after_re_base.slt.end());
                uint32_t max_recv_load_re_base = *max_element(cur_lt_after_re_base.rlt.begin(), cur_lt_after_re_base.rlt.end());

                for (uint16_t encode_node_id = 0; encode_node_id < num_nodes; encode_node_id++)
                {
                    // send load: subtract the locally stored data blocks to send, and add the parity blocks for distribution
                    uint8_t num_transferred_data_blocks = code.k_f - stripe_group.data_dist[encode_node_id];
                    uint32_t send_load_at_node = cur_lt_after_re_base.slt[encode_node_id] - stripe_group.data_dist[encode_node_id] + code.m_f;

                    // recv load: add the data blocks received
                    uint32_t recv_load_at_node = cur_lt_after_re_base.rlt[encode_node_id] + num_transferred_data_blocks;

                    // maximum load and bandwidth
                    uint32_t max_load = max(max(max_send_load_re_base, max_recv_load_re_base), max(send_load_at_node, recv_load_at_node));
                    uint32_t bandwidth = num_transferred_data_blocks + code.m_f;

                    // update min_max_load and min_bw
                    if (max_load <= min_max_load)
                    {
                        // have improved max_load or bandwidth
                        if (max_load < min_max_load || (max_load == min_max_load && bandwidth < min_bw))
                        { // update the results
                            min_max_load = max_load;
                            min_bw = bandwidth;
                            best_lts.clear();
                        }
                        if (bandwidth <= min_bw)
                        {
                            LoadTable cand_lt = stripe_group.genPartialLT4ParityCompute(EncodeMethod::RE_ENCODE, u16string(code.m_f, encode_node_id));
                            best_lts.push_back(move(cand_lt));
                        }
                    }
                }
            }

            if (approach == "BTPM" || approach == "BT")
            { // parity merging
                // base load table for parity merging (after adding to cur_lt)
                LoadTable cur_lt_after_pm_base = cur_lt_after;

                // add all parity blocks to form the base pm solution
                for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                {
                    Utils::dotAddVectors(cur_lt_after_pm_base.slt, stripe_group.parity_dists[parity_id], cur_lt_after_pm_base.slt);

                    // for (auto &stripe : stripe_group.pre_stripes)
                    // {
                    //     uint16_t parity_placed_node = stripe->indices[code.k_i + parity_id];
                    //     cur_lt_after_pm_base.slt[parity_placed_node]++;
                    // }
                }

                uint32_t num_pm_lts = pow(num_nodes, code.m_f);
                u16string pm_nodes(code.m_f, 0); // computation for parity i is at pm_nodes[i]
                for (uint32_t perm_id = 0; perm_id < num_pm_lts; perm_id++)
                {
                    LoadTable cur_lt_after_pm = cur_lt_after_pm_base;

                    // obtain temp block placement (with data blocks and newly computed parity blocks)
                    u16string temp_block_placement = stripe_group.data_dist;
                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        temp_block_placement[pm_nodes[parity_id]]++;
                    }

                    // bandwidth for the current solution
                    uint32_t bandwidth = code.lambda_i * code.m_f;

                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        uint16_t parity_compute_node = pm_nodes[parity_id];

                        // update the base send load table
                        // subtract the parity blocks locally stored at the node
                        uint8_t num_stored_parity_cmp_node = stripe_group.parity_dists[parity_id][parity_compute_node];
                        cur_lt_after_pm.slt[parity_compute_node] -= num_stored_parity_cmp_node;
                        bandwidth -= num_stored_parity_cmp_node;

                        // check if parity block relocation is needed
                        if (temp_block_placement[parity_compute_node] > 1)
                        {
                            cur_lt_after_pm.slt[parity_compute_node]++;
                            bandwidth++;
                        }

                        // update the base recv load table
                        // add the number of parity blocks received
                        cur_lt_after_pm.rlt[parity_compute_node] += (code.lambda_i - num_stored_parity_cmp_node);
                    }

                    // maximum load for the current solution
                    uint32_t max_send_load_pm = *max_element(cur_lt_after_pm.slt.begin(), cur_lt_after_pm.slt.end());
                    uint32_t max_recv_load_pm = *max_element(cur_lt_after_pm.rlt.begin(), cur_lt_after_pm.rlt.end());
                    uint32_t max_load = max(max_send_load_pm, max_recv_load_pm);

                    // update min_max_load and min_bw
                    if (max_load <= min_max_load)
                    {
                        // have improved result
                        if (max_load < min_max_load || (max_load == min_max_load && bandwidth < min_bw))
                        {
                            min_max_load = max_load;
                            min_bw = bandwidth;
                            best_lts.clear();
                        }
                        if (bandwidth <= min_bw)
                        {
                            LoadTable cand_lt = stripe_group.genPartialLT4ParityCompute(EncodeMethod::PARITY_MERGE, pm_nodes);
                            best_lts.push_back(move(cand_lt));
                        }
                    }

                    // get next permutation
                    Utils::getNextPerm(num_nodes, code.m_f, pm_nodes);
                }
            }

            // if (best_lts.size() == 0)
            // {
            //     printf("no better solution for this stripe group\n");
            //     continue;
            // }

            // randomly choose a best load table
            size_t random_pos = Utils::randomUInt(0, best_lts.size() - 1, random_generator);
            LoadTable &selected_lt = best_lts[random_pos];

            // update current load table
            if (stripe_group.applied_lt.approach != EncodeMethod::UNKNOWN_METHOD)
            {
                // if we already find and applied a load table for the stripe group, need to subtract it
                Utils::dotSubVectors(cur_lt.slt, stripe_group.applied_lt.slt, cur_lt.slt);
                Utils::dotSubVectors(cur_lt.rlt, stripe_group.applied_lt.rlt, cur_lt.rlt);
                cur_lt.bw -= stripe_group.applied_lt.bw;
            }

            Utils::dotAddVectors(cur_lt.slt, selected_lt.slt, cur_lt.slt);
            Utils::dotAddVectors(cur_lt.rlt, selected_lt.rlt, cur_lt.rlt);
            cur_lt.bw += selected_lt.bw;

            // apply the load table for stripe group
            stripe_group.applied_lt = selected_lt;

            // free up the memory
            stripe_group.cand_partial_lts.resize(0);
            stripe_group.cand_partial_lts.shrink_to_fit();

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

        // printf("cur_lt after iteration %lu:\n", iter);
        // printf("send load: ");
        // Utils::printVector(cur_lt.slt);
        // printf("recv load: ");
        // Utils::printVector(cur_lt.rlt);
        // printf("bandwidth: %u\n", cur_lt.bw);
    }

    // update the metadata for stripe group
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;
        stripe_group.parity_comp_method = stripe_group.applied_lt.approach;
        stripe_group.parity_comp_nodes = stripe_group.applied_lt.enc_nodes;

        // clear the candidate load tables for memory saving
        stripe_group.cand_partial_lts.clear();
    }

    uint32_t num_re_groups = 0;
    for (auto &item : stripe_batch.selected_sgs)
    {
        num_re_groups += (item.second.applied_lt.approach == EncodeMethod::RE_ENCODE ? 1 : 0);
    }

    gettimeofday(&end_time, nullptr);
    finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("finished finding parity computation scheme for %lu stripe groups, time: %f ms\n", stripe_batch.selected_sgs.size(), finish_time);

    printf("final load table with data relocation (send load only), parity generation (both send and receive load) and parity relocation (send load only):\n");
    printf("send load: ");
    Utils::printVector(cur_lt.slt);
    printf("recv load: ");
    Utils::printVector(cur_lt.rlt);
    printf("bandwidth: %u\n", cur_lt.bw);
    printf("number of re-encoding groups: (%u / %lu), number of parity merging groups: (%lu / %lu)\n", num_re_groups, stripe_batch.selected_sgs.size(), (stripe_batch.selected_sgs.size() - num_re_groups), stripe_batch.selected_sgs.size());
}

void BalancedConversion::genParityComputationGreedyOptimized(StripeBatch &stripe_batch, string approach)
{
    ConvertibleCode &code = stripe_batch.code;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    double finish_time = 0.0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    // maintain a load table for currently selected stripe groups
    LoadTable cur_lt;
    cur_lt.slt.assign(num_nodes, 0);
    cur_lt.rlt.assign(num_nodes, 0);

    // initialize the slt with data relocation across all stripe groups
    // given the selected stripe groups, the send load distribution for data relocation is deterministic, as we can obtain the data placement and know how many data blocks needs to be relocated
    // Note: in this step, the receive load for data relocation is not determined, and will be scheduled in Step 4
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

    // bw is calculated with data relocation
    cur_lt.bw = accumulate(cur_lt.slt.begin(), cur_lt.slt.end(), 0);

    // printf("current_lt initialized with data relocation send load:\n");
    // printf("send load: ");
    // Utils::printVector(cur_lt.slt);
    // printf("recv load: ");
    // Utils::printVector(cur_lt.rlt);
    // printf("bandwidth: %u\n", cur_lt.bw);

    // check whether the stripe can be perfectly merged by parity merging
    vector<bool> is_sg_perfect_pm(stripe_batch.selected_sgs.size(), false);
    if (approach == "BTPM" || approach == "BT")
    {
        for (auto &item : stripe_batch.selected_sgs)
        {
            uint32_t sg_id = item.first;
            StripeGroup &stripe_group = item.second;

            // before the enumeration, check the bandwidth for parity computation; if parity generation satisfy perfect parity merging (bw = 0), then directly apply the scheme
            if (stripe_group.isPerfectParityMerging() == true)
            {
                // generate parity computation scheme for parity merging
                stripe_group.genParityComputeScheme4PerfectPM();
                is_sg_perfect_pm[sg_id] = true;
                continue;
            }
        }
    }

    // use a while loop until the solution cannot be further optimized (i.e.,
    // cannot further improve load balance and then bandwidth and then cv)

    // metric for choosing load tables: (1) min max load; (2) bw; (3) cv
    uint64_t iter = 0;
    uint32_t cur_max_load_iter = UINT32_MAX;
    uint32_t cur_bw_iter = UINT32_MAX;
    double cur_cv_iter = UINT32_MAX;

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

            // based on the send load from data relocation, for each stripe group, generate all possible solutions for parity computation (for re-encoding and/or parity merging) and finds the most load-balanced solution (which determines the parity generation method and node)

            // metric for choosing load tables: (1) minimize max load: max(max(cur_lt.slt + slt), max(cur_lt.rlt + rlt)); if (1) finds multiple load tables, choose the load table with minimum bandwidth; if (2) also finds multiple lts, randomly pick one

            vector<LoadTable> best_lts;

            if (approach == "BTRE" || approach == "BT")
            { // re-encoding

                uint32_t min_max_load_sg = UINT32_MAX;
                uint32_t min_bw_sg = UINT8_MAX;

                LoadTable cur_lt_after_re_base = cur_lt;

                // if we already find and applied a load table for the stripe group, need to subtract it before changing the solution
                if (stripe_group.applied_lt.approach != EncodeMethod::UNKNOWN_METHOD)
                {
                    Utils::dotSubVectors(cur_lt_after_re_base.slt, stripe_group.applied_lt.slt, cur_lt_after_re_base.slt);
                    Utils::dotSubVectors(cur_lt_after_re_base.rlt, stripe_group.applied_lt.rlt, cur_lt_after_re_base.rlt);
                }

                // add data distribution to form the base re-encoding solution
                Utils::dotAddVectors(cur_lt_after_re_base.slt, stripe_group.data_dist, cur_lt_after_re_base.slt);

                // maximum send load with base solution for re-encoding
                uint32_t max_send_load_re_base = *max_element(cur_lt_after_re_base.slt.begin(), cur_lt_after_re_base.slt.end());
                uint32_t max_recv_load_re_base = *max_element(cur_lt_after_re_base.rlt.begin(), cur_lt_after_re_base.rlt.end());

                for (uint16_t encode_node_id = 0; encode_node_id < num_nodes; encode_node_id++)
                {
                    // send load: subtract the locally stored data blocks to send, and add the parity blocks for distribution
                    uint8_t num_transferred_data_blocks = code.k_f - stripe_group.data_dist[encode_node_id];
                    uint32_t send_load_at_node = cur_lt_after_re_base.slt[encode_node_id] - stripe_group.data_dist[encode_node_id] + code.m_f;

                    // recv load: add the data blocks received
                    uint32_t recv_load_at_node = cur_lt_after_re_base.rlt[encode_node_id] + num_transferred_data_blocks;

                    // maximum load and bandwidth
                    uint32_t max_load = max(max(max_send_load_re_base, max_recv_load_re_base), max(send_load_at_node, recv_load_at_node));
                    uint32_t bandwidth = num_transferred_data_blocks + code.m_f;

                    // update min_max_load_sg and min_bw_sg
                    if (max_load <= min_max_load_sg)
                    {
                        // have improved max_load or bandwidth
                        if (max_load < min_max_load_sg || (max_load == min_max_load_sg && bandwidth < min_bw_sg))
                        { // update the results
                            min_max_load_sg = max_load;
                            min_bw_sg = bandwidth;
                            best_lts.clear();
                        }
                        if (bandwidth <= min_bw_sg)
                        {
                            LoadTable cand_lt = stripe_group.genPartialLT4ParityCompute(EncodeMethod::RE_ENCODE, u16string(code.m_f, encode_node_id));
                            best_lts.push_back(move(cand_lt));
                        }
                    }
                }
            }

            if (approach == "BTPM" || approach == "BT")
            { // parity merging

                // we can greedily choosing the best solution for each parity
                // block to reduce the computation overhead (total: M *
                // code.m_f solutions)
                u16string pm_nodes(code.m_f, INVALID_NODE_ID); // computation for parity i is at pm_nodes[i]

                // record current block placement for selected parities
                u16string cur_block_placement = stripe_group.data_dist;

                // if the stripe group has already found a solution (for all
                // encoding nodes), record them with the corresponding placements
                if (stripe_group.applied_lt.approach != EncodeMethod::UNKNOWN_METHOD)
                {
                    pm_nodes = stripe_group.applied_lt.enc_nodes;

                    for (auto pm_node : pm_nodes)
                    {
                        cur_block_placement[pm_node]++;
                    }
                }

                // cur_lt_sg_wo_pm: temp load table to store the load without parity merging
                LoadTable cur_lt_sg_wo_pm = cur_lt;

                // check the current solution of each parity block
                for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                {
                    // DEBUG: record max load
                    uint32_t max_load_slt_before = *max_element(cur_lt_sg_wo_pm.slt.begin(), cur_lt_sg_wo_pm.slt.end());
                    uint32_t max_load_rlt_before = *max_element(cur_lt_sg_wo_pm.rlt.begin(), cur_lt_sg_wo_pm.rlt.end());
                    uint32_t max_load_before = max(max_load_slt_before, max_load_rlt_before);

                    uint16_t cur_pm_node = pm_nodes[parity_id]; // current pm node

                    // if there is already a solution, subtract the load for
                    // the current solution of this parity block
                    if (cur_pm_node != INVALID_NODE_ID)
                    {
                        // printf("stripe group: %u, parity: %u, original_node: %u, start minus:\n", sg_id, parity_id, cur_pm_node);
                        // Utils::printVector(cur_lt_sg_wo_pm.slt);
                        // Utils::printVector(cur_lt_sg_wo_pm.rlt);

                        // subtract the parity blocks to send
                        for (auto &stripe : stripe_group.pre_stripes)
                        {
                            uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                            if (parity_stored_node != cur_pm_node)
                            {
                                cur_lt_sg_wo_pm.slt[parity_stored_node]--;
                            }
                        }

                        // substract the blocks to relocate
                        if (cur_block_placement[cur_pm_node] > 1)
                        {
                            cur_lt_sg_wo_pm.slt[cur_pm_node]--;
                        }

                        // subtract the recv load table
                        uint8_t num_parity_blocks_stored_pm_node = stripe_group.parity_dists[parity_id][cur_pm_node];
                        cur_lt_sg_wo_pm.rlt[cur_pm_node] -= (code.lambda_i - num_parity_blocks_stored_pm_node);

                        // reset the placement
                        cur_block_placement[cur_pm_node]--;

                        // reset pm node
                        pm_nodes[parity_id] = INVALID_NODE_ID;

                        // DEBUG: record max load
                        uint32_t max_load_slt_middle = *max_element(cur_lt_sg_wo_pm.slt.begin(), cur_lt_sg_wo_pm.slt.end());
                        uint32_t max_load_rlt_middle = *max_element(cur_lt_sg_wo_pm.rlt.begin(), cur_lt_sg_wo_pm.rlt.end());
                        uint32_t max_load_middle = max(max_load_slt_middle, max_load_rlt_middle);
                    }

                    // record the best results (min_max_load, min_bw_pm,
                    // min_cv, best_nodes)
                    uint32_t min_max_load_sg_wi_pm = UINT32_MAX;
                    uint8_t min_bw_pm_parity = UINT8_MAX;
                    double min_cv_sg_wi_pm = UINT32_MAX;
                    vector<pair<uint16_t, LoadTable>> best_nodes_for_pm;

                    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
                    {
                        uint16_t cand_pm_node = node_id;

                        // record cur_lt_sg_wi_pm after compute the parity block at node_id
                        LoadTable cur_lt_sg_wi_pm = cur_lt_sg_wo_pm;

                        uint8_t num_parity_blocks_stored_pm_node = stripe_group.parity_dists[parity_id][cand_pm_node];

                        // bandwidth for the current solution
                        uint8_t bw_pm_parity = code.lambda_i - num_parity_blocks_stored_pm_node;

                        // update the send load table
                        // add the parity blocks to send
                        for (auto &stripe : stripe_group.pre_stripes)
                        {
                            uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                            if (parity_stored_node != cand_pm_node)
                            {
                                cur_lt_sg_wi_pm.slt[parity_stored_node]++;
                            }
                        }

                        if (cur_block_placement[cand_pm_node] > 0)
                        { // check if parity block relocation is needed
                            cur_lt_sg_wi_pm.slt[cand_pm_node]++;
                            bw_pm_parity++;
                        }

                        // update the recv load table
                        cur_lt_sg_wi_pm.rlt[cand_pm_node] += (code.lambda_i - num_parity_blocks_stored_pm_node);

                        // maximum load for the current solution
                        uint32_t max_load_wi_sg_pm_slt = *max_element(cur_lt_sg_wi_pm.slt.begin(), cur_lt_sg_wi_pm.slt.end());
                        uint32_t max_load_wi_sg_pm_rlt = *max_element(cur_lt_sg_wi_pm.rlt.begin(), cur_lt_sg_wi_pm.rlt.end());
                        uint32_t max_load_wi_sg_pm = max(max_load_wi_sg_pm_slt, max_load_wi_sg_pm_rlt);

                        // mean, stddev, cv (send)
                        double mean_load_wi_sg_pm_slt = 1.0 * std::accumulate(cur_lt_sg_wi_pm.slt.begin(), cur_lt_sg_wi_pm.slt.end(), 0) / cur_lt_sg_wi_pm.slt.size();
                        double sqr_wi_sg_pm_slt = 0;
                        for (auto &item : cur_lt_sg_wi_pm.slt)
                        {
                            sqr_wi_sg_pm_slt += pow((double)item - mean_load_wi_sg_pm_slt, 2);
                        }
                        double dev_wi_sg_pm_slt = sqr_wi_sg_pm_slt / cur_lt_sg_wi_pm.slt.size();
                        double stddev_wi_sg_pm_slt = std::sqrt(dev_wi_sg_pm_slt);
                        double cv_wi_sg_pm_slt = stddev_wi_sg_pm_slt / mean_load_wi_sg_pm_slt;
                        double load_imb_slt = (max_load_wi_sg_pm_slt - mean_load_wi_sg_pm_slt) / mean_load_wi_sg_pm_slt;

                        // mean, stddev, cv (recv)
                        double mean_load_wi_sg_pm_rlt = 1.0 * std::accumulate(cur_lt_sg_wi_pm.rlt.begin(), cur_lt_sg_wi_pm.rlt.end(), 0) / cur_lt_sg_wi_pm.rlt.size();
                        double sqr_wi_sg_pm_rlt = 0;
                        for (auto &item : cur_lt_sg_wi_pm.rlt)
                        {
                            sqr_wi_sg_pm_rlt += pow((double)item - mean_load_wi_sg_pm_rlt, 2);
                        }
                        double dev_wi_sg_pm_rlt = sqr_wi_sg_pm_rlt / cur_lt_sg_wi_pm.rlt.size();
                        double stddev_wi_sg_pm_rlt = std::sqrt(dev_wi_sg_pm_rlt);
                        double cv_wi_sg_pm_rlt = stddev_wi_sg_pm_rlt / mean_load_wi_sg_pm_rlt;
                        double load_imb_rlt = (max_load_wi_sg_pm_rlt - mean_load_wi_sg_pm_rlt) / mean_load_wi_sg_pm_rlt;

                        // double cv_wi_sg_pm = max(cv_wi_sg_pm_slt, cv_wi_sg_pm_rlt);
                        // double cv_wi_sg_pm = cv_wi_sg_pm_slt;

                        // double cv_wi_sg_pm = max(dev_wi_sg_pm_slt, dev_wi_sg_pm_rlt);
                        double cv_wi_sg_pm = max(stddev_wi_sg_pm_slt, stddev_wi_sg_pm_rlt);
                        // double cv_wi_sg_pm = stddev_wi_sg_pm_slt;

                        // double cv_wi_sg_pm = max(load_imb_slt, load_imb_rlt);
                        // double cv_wi_sg_pm = load_imb_slt;

                        bool is_preserved = max_load_wi_sg_pm < min_max_load_sg_wi_pm || (max_load_wi_sg_pm == min_max_load_sg_wi_pm && bw_pm_parity < min_bw_pm_parity) || (max_load_wi_sg_pm == min_max_load_sg_wi_pm && bw_pm_parity == min_bw_pm_parity && cv_wi_sg_pm <= min_cv_sg_wi_pm);

                        if (is_preserved)
                        {
                            best_nodes_for_pm.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_sg_wi_pm));
                        }

                        // have improved result
                        bool is_improved = max_load_wi_sg_pm < min_max_load_sg_wi_pm || (max_load_wi_sg_pm == min_max_load_sg_wi_pm && bw_pm_parity < min_bw_pm_parity) || (max_load_wi_sg_pm == min_max_load_sg_wi_pm && bw_pm_parity == min_bw_pm_parity && cv_wi_sg_pm < min_cv_sg_wi_pm);

                        if (is_improved == true)
                        {
                            min_max_load_sg_wi_pm = max_load_wi_sg_pm;
                            min_bw_pm_parity = bw_pm_parity;
                            min_cv_sg_wi_pm = cv_wi_sg_pm;
                            best_nodes_for_pm.clear();

                            best_nodes_for_pm.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_sg_wi_pm));
                        }
                    }

                    // printf("num_choices: %u\n", best_nodes_for_pm.size());

                    // randomly choose a best parity compute node
                    size_t random_pos = Utils::randomUInt(0, best_nodes_for_pm.size() - 1, random_generator);
                    uint32_t selected_parity_compute_node = best_nodes_for_pm[random_pos].first;
                    LoadTable &selected_cur_lt_sg_wi_pm = best_nodes_for_pm[random_pos].second;

                    // update metadata
                    pm_nodes[parity_id] = selected_parity_compute_node;  // choose to compute parity block at the node
                    cur_lt_sg_wo_pm = selected_cur_lt_sg_wi_pm;          // update the current load table
                    cur_block_placement[selected_parity_compute_node]++; // place the computed parity block at the node

                    // DEBUG: record previous max load
                    uint32_t max_load_slt_after = *max_element(cur_lt_sg_wo_pm.slt.begin(), cur_lt_sg_wo_pm.slt.end());
                    uint32_t max_load_rlt_after = *max_element(cur_lt_sg_wo_pm.rlt.begin(), cur_lt_sg_wo_pm.rlt.end());
                    uint32_t max_load_after = max(max_load_slt_after, max_load_rlt_after);
                }

                LoadTable cand_lt = stripe_group.genPartialLT4ParityCompute(EncodeMethod::PARITY_MERGE, pm_nodes);
                best_lts.push_back(move(cand_lt));
            }

            // if (best_lts.size() == 0)
            // {
            //     printf("no better solution for this stripe group\n");
            //     continue;
            // }

            // randomly choose a best load table
            size_t random_pos = Utils::randomUInt(0, best_lts.size() - 1, random_generator);
            LoadTable &selected_lt = best_lts[random_pos];

            // update current load table
            if (stripe_group.applied_lt.approach != EncodeMethod::UNKNOWN_METHOD)
            {
                // if we already find and applied a load table for the stripe group, need to subtract it
                Utils::dotSubVectors(cur_lt.slt, stripe_group.applied_lt.slt, cur_lt.slt);
                Utils::dotSubVectors(cur_lt.rlt, stripe_group.applied_lt.rlt, cur_lt.rlt);
                cur_lt.bw -= stripe_group.applied_lt.bw;
            }

            Utils::dotAddVectors(cur_lt.slt, selected_lt.slt, cur_lt.slt);
            Utils::dotAddVectors(cur_lt.rlt, selected_lt.rlt, cur_lt.rlt);
            cur_lt.bw += selected_lt.bw;

            // apply the load table for stripe group
            stripe_group.applied_lt = selected_lt;
        }

        // summarize the current bw
        uint32_t cur_max_send_load = *max_element(cur_lt.slt.begin(), cur_lt.slt.end());
        uint32_t cur_max_recv_load = *max_element(cur_lt.rlt.begin(), cur_lt.rlt.end());
        uint32_t cur_max_load = max(cur_max_send_load, cur_max_recv_load);

        // mean, stddev, cv
        double mean_load_slt = 1.0 * std::accumulate(cur_lt.slt.begin(), cur_lt.slt.end(), 0) / cur_lt.slt.size();
        double sqr_sum_load_slt = 0;
        for (auto &item : cur_lt.slt)
        {
            sqr_sum_load_slt += pow((double)item - mean_load_slt, 2);
        }
        double dev_load_slt = sqr_sum_load_slt / cur_lt.slt.size();
        double stddev_load_slt = std::sqrt(dev_load_slt);
        double cv_slt = stddev_load_slt / mean_load_slt;
        double load_imb_slt = (cur_max_send_load - mean_load_slt) / mean_load_slt;

        // mean, stddev, cv
        double mean_load_rlt = 1.0 * std::accumulate(cur_lt.rlt.begin(), cur_lt.rlt.end(), 0) / cur_lt.rlt.size();
        double sqr_sum_load_rlt = 0;
        for (auto &item : cur_lt.rlt)
        {
            sqr_sum_load_rlt += pow((double)item - mean_load_rlt, 2);
        }
        double dev_load_rlt = sqr_sum_load_rlt / cur_lt.rlt.size();
        double stddev_load_rlt = std::sqrt(dev_load_rlt);
        double cv_rlt = stddev_load_rlt / mean_load_rlt;
        double load_imb_rlt = (cur_max_recv_load - mean_load_rlt) / mean_load_rlt;

        // max cv of send and recv respectively
        // double cv_lt = cv_slt;
        // double cv_lt = max(cv_slt, cv_rlt);

        // double cv_lt = stddev_load_slt;
        // double cv_lt = max(dev_load_slt, dev_load_rlt);
        double cv_lt = max(stddev_load_slt, stddev_load_rlt);

        // double cv_lt = max(load_imb_slt, load_imb_rlt);
        // double cv_lt = load_imb_slt;

        bool improved = cur_max_load < cur_max_load_iter || (cur_max_load == cur_max_load_iter && cur_lt.bw < cur_bw_iter) || (cur_max_load == cur_max_load_iter && cur_lt.bw == cur_bw_iter && cv_lt < cur_cv_iter);
        if (improved == true)
        {
            // update the max load and bw
            cur_max_load_iter = cur_max_load;
            cur_bw_iter = cur_lt.bw;
            cur_cv_iter = cv_lt;
        }
        else
        {
            // the solution cannot be further optimized
            break;
        }

        // printf("cur_lt after iteration %lu:\n", iter);
        // printf("send load: ");
        // Utils::printVector(cur_lt.slt);
        // printf("recv load: ");
        // Utils::printVector(cur_lt.rlt);
        // printf("bandwidth: %u\n", cur_lt.bw);
    }

    // update the metadata for stripe group
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;
        stripe_group.parity_comp_method = stripe_group.applied_lt.approach;
        stripe_group.parity_comp_nodes = stripe_group.applied_lt.enc_nodes;

        // clear the candidate load tables for memory saving
        stripe_group.cand_partial_lts.clear();
    }

    uint32_t num_re_groups = 0;
    for (auto &item : stripe_batch.selected_sgs)
    {
        num_re_groups += (item.second.applied_lt.approach == EncodeMethod::RE_ENCODE ? 1 : 0);
    }

    gettimeofday(&end_time, nullptr);
    finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("finished finding parity computation scheme for %lu stripe groups, time: %f ms\n", stripe_batch.selected_sgs.size(), finish_time);

    printf("final load table with data relocation (send load only), parity generation (both send and receive load) and parity relocation (send load only):\n");
    printf("send load: ");
    Utils::printVector(cur_lt.slt);
    printf("recv load: ");
    Utils::printVector(cur_lt.rlt);
    printf("bandwidth: %u\n", cur_lt.bw);
    printf("number of re-encoding groups: (%u / %lu), number of parity merging groups: (%lu / %lu)\n", num_re_groups, stripe_batch.selected_sgs.size(), (stripe_batch.selected_sgs.size() - num_re_groups), stripe_batch.selected_sgs.size());
}

void BalancedConversion::genBlockRelocation(StripeBatch &stripe_batch)
{
    // NOTE: before this step, the solution of parity computation (encoding methods and nodes) are fixed; in this step, for each stripe group we schedule the relocation of data and parity blocks by finding nodes to place them

    ConvertibleCode &code = stripe_batch.code;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    double finish_time = 0.0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    // step 1: for each stripe group, find the data and parity blocks that needs to be relocated, and available nodes for relocation
    unordered_map<uint32_t, u16string> sg_final_block_placement;                 // for each stripe group, record the block placement (node where it placed) in the final stripe; size: code.n_f
    unordered_map<uint32_t, vector<pair<uint8_t, uint16_t>>> sg_blocks_to_reloc; // for each block in the final stripe that needs to be relocated, record <final_block_id, currently stored node>
    unordered_map<uint32_t, vector<uint16_t>> sg_avail_nodes;

    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        // init block placement
        sg_final_block_placement[sg_id].assign(code.n_f, INVALID_NODE_ID);

        // record is node relocated with a block
        vector<bool> is_node_relocated(num_nodes, false);

        // find the blocks to be relocated
        for (uint8_t final_block_id = 0; final_block_id < code.n_f; final_block_id++)
        {
            uint16_t cur_placed_node_id = INVALID_NODE_ID;
            if (final_block_id < code.k_f)
            { // data block
                uint8_t stripe_id = final_block_id / code.k_i;
                uint8_t block_id = final_block_id % code.k_i;

                cur_placed_node_id = stripe_group.pre_stripes[stripe_id]->indices[block_id];
            }
            else
            { // parity block
                uint8_t parity_id = final_block_id - code.k_f;
                cur_placed_node_id = stripe_group.parity_comp_nodes[parity_id];
            }

            sg_final_block_placement[sg_id][final_block_id] = cur_placed_node_id;

            // check if the node has been placed with a block
            if (is_node_relocated[cur_placed_node_id] == false)
            {
                // directly place the block at the node
                is_node_relocated[cur_placed_node_id] = true; // mark the node as relocated
            }
            else
            {
                // mark the block as needed to be relocated
                sg_blocks_to_reloc[sg_id].push_back(pair<uint8_t, uint16_t>(final_block_id, cur_placed_node_id));
            }
        }

        // find available nodes to relocate the blocks for the stripe group
        for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
        {
            if (is_node_relocated[node_id] == false)
            {
                sg_avail_nodes[sg_id].push_back(node_id);
            }
        }
    }

    // for (auto &item : stripe_batch.selected_sgs)
    // {
    //     uint32_t sg_id = item.first;

    //     printf("sg %u:\n", sg_id);
    //     Utils::printVector(sg_final_block_placement[sg_id]);

    //     printf("available nodes:\n");
    //     Utils::printVector(sg_avail_nodes[sg_id]);
    // }

    // step 2: construct bipartite graph
    Bipartite bipartite;

    unordered_map<uint32_t, uint64_t> node2rvtx_map;              // node to vertex map; each item: <node_id, rvtx_id>
    unordered_map<uint64_t, uint32_t> rvtx2node_map;              // node to vertex map; each item: <rvtx_id, node_id>
    unordered_map<uint64_t, pair<uint32_t, uint8_t>> lvtx2sg_map; // left vertex to stripe group map; each item: <lvtx_id, <sg_id, final_block_id>>
    unordered_map<uint32_t, vector<uint64_t>> sg2lvtx_map;        // stripe group to left vertex map (corresponding to sg_blocks_to_reloc)

    // create right vertices for nodes (rvtx_id <=> node_id)
    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        // for each node, create a right vertex
        uint64_t rvtx_id = bipartite.addVertex(VertexType::RIGHT);
        node2rvtx_map[node_id] = rvtx_id;
        rvtx2node_map[rvtx_id] = node_id;
    }

    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;

        // for each block to relocate, create a left vertex (lvtx_id <=> block_id)
        for (auto &block_info : sg_blocks_to_reloc[sg_id])
        {
            uint64_t lvtx_id = bipartite.addVertex(VertexType::LEFT);

            // record stripe group information for the vertex
            uint8_t final_block_id = block_info.first;
            lvtx2sg_map[lvtx_id] = pair<uint32_t, uint8_t>(sg_id, final_block_id);
            sg2lvtx_map[sg_id].push_back(lvtx_id);

            // for each available nodes of the stripe group, add an edge
            for (auto avail_node_id : sg_avail_nodes[sg_id])
            {
                bipartite.addEdge(lvtx_id, node2rvtx_map[avail_node_id]);
            }
        }
    }

    // bipartite.print();

    // obtain the initial receive load from parity computation
    LoadTable lt;
    lt.rlt.assign(num_nodes, 0);
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;
        Utils::dotAddVectors(lt.rlt, stripe_group.applied_lt.rlt, lt.rlt);
    }

    // initialize bipartite right vertices in_degrees with the receive load of parity generation
    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        Vertex &rvtx = bipartite.right_vertices_map[node2rvtx_map[node_id]];
        rvtx.in_degree = lt.rlt[node_id];
    }

    printf("finished constructing bipartite graph\n");

    printf("bipartite.left_vertices (size: %ld):\n", bipartite.left_vertices_map.size());
    printf("bipartite.right_vertices (size: %ld):\n", bipartite.right_vertices_map.size());

    // step 3: find optimal semi-matching (based on the initial receive load)
    vector<uint64_t> sm_edges = bipartite.findOptSemiMatching(lvtx2sg_map, sg2lvtx_map);

    printf("finished finding semi-matching solutions\n");

    // update the final_block_placement from chosen edges
    for (auto edge_id : sm_edges)
    {
        Edge &edge = bipartite.edges_map[edge_id];
        Vertex &lvtx = bipartite.left_vertices_map[edge.lvtx_id];
        uint32_t sg_id = lvtx2sg_map[lvtx.id].first;
        uint8_t final_block_id = lvtx2sg_map[lvtx.id].second;
        uint16_t reloc_node_id = rvtx2node_map[edge.rvtx_id];

        sg_final_block_placement[sg_id][final_block_id] = reloc_node_id; // record place node_id
    }

    // step 4: update placement metadata
    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;
        stripe_group.post_stripe->indices = sg_final_block_placement[sg_id];
    }

    gettimeofday(&end_time, nullptr);
    finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("finished finding block relocation scheme for %lu stripe groups, time: %f ms\n", stripe_batch.selected_sgs.size(), finish_time);
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