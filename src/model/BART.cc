#include "BART.hh"

BART::BART(mt19937 &_random_generator) : random_generator(_random_generator)
{
}

BART::~BART()
{
}

void BART::genSolution(StripeBatch &stripe_batch, string approach)
{
    // Step 1: construct stripe groups
    printf("Step 1: stripe group construction\n");
    // stripe_batch.constructSGByBWBF(approach);
    stripe_batch.constructSGByBWPartial(approach);
    // stripe_batch.print();

    // Step 2: generate parity computation scheme (parity computation method and nodes)
    printf("Step 2: parity block generation\n");
    // genParityComputationHybrid(stripe_batch, approach);
    if (approach == "BTWeighted")
    {
        genWeightedParityGenerationForPM(stripe_batch);
    }
    else
    {
        genParityGenerationForPM(stripe_batch);
    }

    // Step 3: schedule (data and parity) block relocation
    printf("Step 3: stripe redistribution\n");

    if (approach == "BTWeighted")
    {
        genWeightedStripeRedistribution(stripe_batch);
    }
    else
    {
        genStripeRedistribution(stripe_batch);
    }
}

void BART::genParityGenerationForPM(StripeBatch &stripe_batch)
{
    double finish_time = 0.0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    ConvertibleCode &code = stripe_batch.code;
    auto &stripe_groups = stripe_batch.selected_sgs;
    uint32_t num_stripe_groups = stripe_groups.size();

    // initialize the applied load tables for each stripe group
    for (auto &item : stripe_groups)
    {
        StripeGroup &stripe_group = item.second;

        stripe_group.applied_lt.approach = PARITY_MERGE;
        stripe_group.applied_lt.enc_nodes.assign(code.m_f, INVALID_NODE_ID);
    }

    // maintain a load table for the traffic of all selected encoding nodes of
    // all stripe groups
    LoadTable cur_lt;

    // initialize load table for parity block generation (with data relocation
    // traffic)
    initLTForParityGeneration(stripe_batch, cur_lt);

    // We check each parity block and find whether the parity block can be
    // perfectly merged. If yes, we fix the encoding node for the parity block
    vector<vector<bool>> is_perfect_pm(num_stripe_groups, vector<bool>(code.m_f, false));
    for (auto &item : stripe_groups)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            if (stripe_group.isPerfectPM(parity_id) == true)
            { // the parity block can be perfectly merged
                is_perfect_pm[sg_id][parity_id] = true;
                stripe_group.applied_lt.enc_nodes[parity_id] = stripe_group.genEncNodeForPerfectPM(parity_id);
            }
        }
    }

    // initialize solution of parity block generation
    initSolOfParityGenerationForPM(stripe_batch, is_perfect_pm, cur_lt);

    // optimize solution of parity block generation
    optimizeSolOfParityGenerationForPM(stripe_batch, is_perfect_pm, cur_lt);

    // update the metadata for stripe group
    for (auto &item : stripe_groups)
    {
        StripeGroup &stripe_group = item.second;
        stripe_group.parity_comp_method = stripe_group.applied_lt.approach;
        stripe_group.parity_comp_nodes = stripe_group.applied_lt.enc_nodes;
    }

    // calculate the number of re-encoding groups
    uint32_t num_re_groups = 0;
    for (auto &item : stripe_groups)
    {
        num_re_groups += (item.second.parity_comp_method == EncodeMethod::RE_ENCODE ? 1 : 0);
    }

    gettimeofday(&end_time, nullptr);
    finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("finished finding parity computation scheme for %u stripe groups, time: %f ms\n", num_stripe_groups, finish_time);

    printf("final load table with data relocation (send load only), parity generation (both send and receive load) and parity relocation (send load only):\n");
    printf("send load: ");
    Utils::printVector(cur_lt.slt);
    printf("recv load: ");
    Utils::printVector(cur_lt.rlt);
    printf("bandwidth: %u\n", cur_lt.bw);
    printf("number of re-encoding groups: (%u / %u), number of parity merging groups: (%u / %u)\n", num_re_groups, num_stripe_groups, (num_stripe_groups - num_re_groups), num_stripe_groups);
}

void BART::genWeightedParityGenerationForPM(StripeBatch &stripe_batch)
{
    double finish_time = 0.0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    ConvertibleCode &code = stripe_batch.code;
    ClusterSettings &settings = stripe_batch.settings;
    auto &stripe_groups = stripe_batch.selected_sgs;
    uint32_t num_stripe_groups = stripe_groups.size();

    // initialize the applied load tables for each stripe group
    for (auto &item : stripe_groups)
    {
        StripeGroup &stripe_group = item.second;

        stripe_group.applied_lt.approach = PARITY_MERGE;
        stripe_group.applied_lt.enc_nodes.assign(code.m_f, INVALID_NODE_ID);
    }

    // maintain a load table for the traffic of all selected encoding nodes of
    // all stripe groups
    LoadTable cur_lt;

    // initialize load table for parity block generation (with data relocation
    // traffic)
    initLTForParityGeneration(stripe_batch, cur_lt);

    // We check each parity block and find whether the parity block can be
    // perfectly merged. If yes, we fix the encoding node for the parity block
    vector<vector<bool>> is_perfect_pm(num_stripe_groups, vector<bool>(code.m_f, false));
    for (auto &item : stripe_groups)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            if (stripe_group.isPerfectPM(parity_id) == true)
            { // the parity block can be perfectly merged
                is_perfect_pm[sg_id][parity_id] = true;
                stripe_group.applied_lt.enc_nodes[parity_id] = stripe_group.genEncNodeForPerfectPM(parity_id);
            }
        }
    }

    // initialize solution of parity block generation
    initWeightedSolOfParityGenerationForPM(stripe_batch, is_perfect_pm, cur_lt);

    // optimize solution of parity block generation
    optimizeWeightedSolOfParityGenerationForPM(stripe_batch, is_perfect_pm, cur_lt);

    // update the metadata for stripe group
    for (auto &item : stripe_groups)
    {
        StripeGroup &stripe_group = item.second;
        stripe_group.parity_comp_method = stripe_group.applied_lt.approach;
        stripe_group.parity_comp_nodes = stripe_group.applied_lt.enc_nodes;
    }

    // calculate the number of re-encoding groups
    uint32_t num_re_groups = 0;
    for (auto &item : stripe_groups)
    {
        num_re_groups += (item.second.parity_comp_method == EncodeMethod::RE_ENCODE ? 1 : 0);
    }

    gettimeofday(&end_time, nullptr);
    finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("finished finding parity computation scheme for %u stripe groups, time: %f ms\n", num_stripe_groups, finish_time);

    printf("final weighted load table with data relocation (send load only), parity generation (both send and receive load) and parity relocation (send load only):\n");

    // weighted load table
    vector<double> weighted_slt(settings.num_nodes);
    vector<double> weighted_rlt(settings.num_nodes);
    for (uint16_t node_id = 0; node_id < settings.num_nodes; node_id++)
    {
        // weighted_load = load / bw
        weighted_slt[node_id] = 1.0 * cur_lt.slt[node_id] / settings.bw_profile.upload[node_id];
        weighted_rlt[node_id] = 1.0 * cur_lt.rlt[node_id] / settings.bw_profile.download[node_id];
    }
    printf("final weighted load table:\n");
    printf("send load: ");
    Utils::printVector(weighted_slt);
    printf("recv load: ");
    Utils::printVector(weighted_rlt);

    printf("bandwidth: %u\n", cur_lt.bw);
    printf("number of re-encoding groups: (%u / %u), number of parity merging groups: (%u / %u)\n", num_re_groups, num_stripe_groups, (num_stripe_groups - num_re_groups), num_stripe_groups);
}

void BART::initLTForParityGeneration(StripeBatch &stripe_batch, LoadTable &cur_lt)
{
    ConvertibleCode &code = stripe_batch.code;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    // initialize current load table
    cur_lt.slt.assign(num_nodes, 0);
    cur_lt.rlt.assign(num_nodes, 0);

    // initialize the send load table with data relocation + retrieving original parity block traffic
    // we check the stripe placements from the stripe groups and count how
    // many data blocks needs to be relocated + how many original parity
    // blocks needs to be sent
    // NOTE: the receive load for data relocation and parity relocation is not
    // determined, and will be scheduled after parity block generation
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;

        // mark if each node is placed with a data block
        vector<bool> is_node_placed(num_nodes, false);

        for (auto &stripe : stripe_group.pre_stripes)
        {
            for (uint8_t block_id = 0; block_id < code.k_i; block_id++)
            {
                uint16_t src_data_node = stripe->indices[block_id];
                if (is_node_placed[src_data_node] == false)
                {
                    is_node_placed[src_data_node] = true;
                }
                else
                { // if the node is already placed with a data block, then it needs to be relocated
                    cur_lt.slt[src_data_node]++;
                }
            }
        }

        // sum up all original parity block (assume we need to retrieve all of them)
        for (auto &stripe : stripe_group.pre_stripes)
        {
            for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
            {
                uint16_t src_parity_node = stripe->indices[code.k_i + parity_id];
                cur_lt.slt[src_parity_node]++;
            }
        }
    }

    // calculate bandwidth with data relocation (for send load table)
    cur_lt.bw = accumulate(cur_lt.slt.begin(), cur_lt.slt.end(), 0);

    printf("initialization of load table, cur_lt:\n");
    printf("send load: ");
    Utils::printVector(cur_lt.slt);
    printf("recv load: ");
    Utils::printVector(cur_lt.rlt);
    printf("bandwidth: %u\n", cur_lt.bw);
}

void BART::initLTForParityGenerationData(StripeBatch &stripe_batch, LoadTable &cur_lt)
{
    ConvertibleCode &code = stripe_batch.code;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    // initialize current load table
    cur_lt.slt.assign(num_nodes, 0);
    cur_lt.rlt.assign(num_nodes, 0);

    // initialize the send load table with data relocation traffic
    // we check the stripe placements from the stripe groups and count how many data blocks needs to be relocated
    // NOTE: the receive load for data relocation is not determined, and will
    // be scheduled after parity block generation
    for (auto &item : stripe_batch.selected_sgs)
    {
        StripeGroup &stripe_group = item.second;

        // mark if each node is placed with a data block
        vector<bool> is_node_placed(num_nodes, false);

        for (auto &stripe : stripe_group.pre_stripes)
        {
            for (uint8_t block_id = 0; block_id < code.k_i; block_id++)
            {
                uint16_t src_data_node = stripe->indices[block_id];
                if (is_node_placed[src_data_node] == false)
                {
                    is_node_placed[src_data_node] = true;
                }
                else
                { // if the node is already placed with a data block, then it needs to be relocated
                    cur_lt.slt[src_data_node]++;
                }
            }
        }
    }

    // calculate bandwidth with data relocation (for send load table)
    cur_lt.bw = accumulate(cur_lt.slt.begin(), cur_lt.slt.end(), 0);

    printf("initialization of load table, cur_lt:\n");
    printf("send load: ");
    Utils::printVector(cur_lt.slt);
    printf("recv load: ");
    Utils::printVector(cur_lt.rlt);
    printf("bandwidth: %u\n", cur_lt.bw);
}

void BART::initSolOfParityGenerationForPM(StripeBatch &stripe_batch, vector<vector<bool>> &is_perfect_pm, LoadTable &cur_lt)
{
    // based on the send load from data relocation, for each parity block,
    // we identify the most load balanced encoding node for parity block
    // generation

    // metric for choosing load tables: (1) minimized max load; if (1)
    // finds multiple encoding nodes, randomly choose a node with minimum
    // bandwidth

    ConvertibleCode &code = stripe_batch.code;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        // record nodes for parity merging
        u16string selected_pm_nodes(code.m_f, INVALID_NODE_ID);

        // record the block placement of each stripe group
        u16string cur_block_placement = stripe_group.data_dist;

        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            // skip parity blocks that can be perfectly merged
            if (is_perfect_pm[sg_id][parity_id] == true)
            {
                uint16_t perfect_pm_node = stripe_group.applied_lt.enc_nodes[parity_id];
                selected_pm_nodes[parity_id] = perfect_pm_node;

                // subtract send load on the perfect_pm_node for retrieving
                // original data blocks
                cur_lt.slt[perfect_pm_node] -= code.lambda_i;
                // subtract additional bandwidth for retrieving original data
                // blocks
                cur_lt.bw -= code.lambda_i;

                continue;
            }

            uint32_t min_max_load_pm = UINT32_MAX;
            uint8_t min_bw_pm = UINT8_MAX;
            vector<pair<uint16_t, LoadTable>> best_pm_nodes;

            for (uint16_t cand_pm_node = 0; cand_pm_node < num_nodes; cand_pm_node++)
            {
                // load table after applying the pm node
                LoadTable cur_lt_after_pm = cur_lt;

                uint8_t num_parities_stored_pm_node = stripe_group.parity_dists[parity_id][cand_pm_node];

                // bandwidth for the current solution
                uint8_t bw_pm = code.lambda_i - num_parities_stored_pm_node;

                // update the send load table
                // subtract the parity blocks to send
                for (auto &stripe : stripe_group.pre_stripes)
                {
                    uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                    if (parity_stored_node == cand_pm_node)
                    {
                        cur_lt_after_pm.slt[parity_stored_node]--;
                    }
                }

                // check if parity block relocation is needed
                if (cur_block_placement[cand_pm_node] > 0)
                {
                    cur_lt_after_pm.slt[cand_pm_node]++;
                    bw_pm++;
                }

                // update the recv load table
                cur_lt_after_pm.rlt[cand_pm_node] += (code.lambda_i - num_parities_stored_pm_node);

                // maximum load for the current solution
                uint32_t max_send_load_pm = *max_element(cur_lt_after_pm.slt.begin(), cur_lt_after_pm.slt.end());
                uint32_t max_recv_load_pm = *max_element(cur_lt_after_pm.rlt.begin(), cur_lt_after_pm.rlt.end());

                // maximum load and bandwidth
                uint32_t max_load_pm = max(max_send_load_pm, max_recv_load_pm);

                // check if the results are preserved
                bool is_preserved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm <= min_bw_pm));
                if (is_preserved == true)
                { // append to the candidate results
                    best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                }

                // check if the results are improved
                bool is_improved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm < min_bw_pm));
                if (is_improved == true)
                {
                    min_max_load_pm = max_load_pm;
                    min_bw_pm = bw_pm;

                    // clear previous results and append the new one
                    best_pm_nodes.clear();
                    best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                }
            }

            // randomly choose a best parity compute node
            size_t random_pos = Utils::randomUInt(0, best_pm_nodes.size() - 1, random_generator);
            uint32_t selected_pm_node = best_pm_nodes[random_pos].first;
            LoadTable &selected_cur_lt_after_pm = best_pm_nodes[random_pos].second;

            // update metadata
            selected_pm_nodes[parity_id] = selected_pm_node; // choose to compute parity block at the node
            cur_lt = selected_cur_lt_after_pm;               // update the current load table

            // subtract the bandwidth to retrieve original parity block
            uint8_t num_parities_stored_pm_node = stripe_group.parity_dists[parity_id][selected_pm_node];

            cur_lt.bw -= num_parities_stored_pm_node;                                 // subtract retrieving original parity block bandwidth
            cur_lt.bw += (min_bw_pm - (code.lambda_i - num_parities_stored_pm_node)); // add new parity block re-distribution bandwidth
            // printf("%u, %u\n", num_parities_stored_pm_node, min_bw_pm - num_parities_stored_pm_node);

            cur_block_placement[selected_pm_node]++; // place the computed parity block at the node

            // printf("stripe group: %u, compute parity %u at node %u, cur_lt: (max_load: %u, bw: %u, bw_sum: %u)\n", stripe_group.id, parity_id, selected_pm_node, min_max_load_pm, min_bw_pm, cur_lt.bw);
            // Utils::printVector(selected_cur_lt_after_pm.slt);
            // Utils::printVector(selected_cur_lt_after_pm.rlt);
        }

        // apply the load table for stripe group
        stripe_group.applied_lt = stripe_group.genPartialLTForParityCompute(EncodeMethod::PARITY_MERGE, selected_pm_nodes);
    }

    gettimeofday(&end_time, nullptr);
    double finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                         (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("finished initialization for %lu stripe groups, time: %f ms\n", stripe_batch.selected_sgs.size(), finish_time);

    printf("find initial solution, cur_lt:\n");
    printf("send load: ");
    Utils::printVector(cur_lt.slt);
    printf("recv load: ");
    Utils::printVector(cur_lt.rlt);
    printf("bandwidth: %u\n", cur_lt.bw);
}

void BART::initWeightedSolOfParityGenerationForPM(StripeBatch &stripe_batch, vector<vector<bool>> &is_perfect_pm, LoadTable &cur_lt)
{
    // based on the send load from data relocation and parity merging, for
    // each parity block, we identify the most load balanced encoding node for
    // parity block generation

    // weighted version: weight_val = (# of blocks transferred / bw_upload or bw_download)

    // metric for choosing load tables: (1) minimized max load; if (1)
    // finds multiple encoding nodes, randomly choose a node with minimum
    // bandwidth

    ConvertibleCode &code = stripe_batch.code;
    ClusterSettings &settings = stripe_batch.settings;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        // record nodes for parity merging
        u16string selected_pm_nodes(code.m_f, INVALID_NODE_ID);

        // record the block placement of each stripe group
        u16string cur_block_placement = stripe_group.data_dist;

        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            // skip parity blocks that can be perfectly merged
            if (is_perfect_pm[sg_id][parity_id] == true)
            {
                uint16_t perfect_pm_node = stripe_group.applied_lt.enc_nodes[parity_id];
                selected_pm_nodes[parity_id] = perfect_pm_node;

                // subtract send load on the perfect_pm_node for retrieving
                // original data blocks
                cur_lt.slt[perfect_pm_node] -= code.lambda_i;
                // subtract additional bandwidth for retrieving original data
                // blocks
                cur_lt.bw -= code.lambda_i;

                continue;
            }

            // min max weighted load
            double min_max_weighted_load_pm = DBL_MAX;
            uint8_t min_bw_pm = UINT8_MAX;
            vector<pair<uint16_t, LoadTable>> best_pm_nodes;

            for (uint16_t cand_pm_node = 0; cand_pm_node < num_nodes; cand_pm_node++)
            {
                // load table after applying the pm node
                LoadTable cur_lt_after_pm = cur_lt;

                uint8_t num_parities_stored_pm_node = stripe_group.parity_dists[parity_id][cand_pm_node];

                // bandwidth for the current solution
                uint8_t bw_pm = code.lambda_i - num_parities_stored_pm_node;

                // update the send load table
                // subtract the parity blocks to send
                for (auto &stripe : stripe_group.pre_stripes)
                {
                    uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                    if (parity_stored_node == cand_pm_node)
                    {
                        cur_lt_after_pm.slt[parity_stored_node]--;
                    }
                }

                // check if parity block relocation is needed
                if (cur_block_placement[cand_pm_node] > 0)
                {
                    cur_lt_after_pm.slt[cand_pm_node]++;
                    bw_pm++;
                }

                // update the recv load table
                cur_lt_after_pm.rlt[cand_pm_node] += (code.lambda_i - num_parities_stored_pm_node);

                // compute maximum weighted load
                double max_weighted_load_pm = 0;
                for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
                {
                    // weighted_load = load / bw
                    double weighted_send_load = 1.0 * cur_lt_after_pm.slt[node_id] / settings.bw_profile.upload[node_id];
                    double weighted_recv_load = 1.0 * cur_lt_after_pm.rlt[node_id] / settings.bw_profile.download[node_id];
                    max_weighted_load_pm = max({max_weighted_load_pm, weighted_send_load, weighted_recv_load});
                }

                // check if the results are preserved
                bool is_preserved = (max_weighted_load_pm < min_max_weighted_load_pm || (max_weighted_load_pm == min_max_weighted_load_pm && bw_pm <= min_bw_pm));
                if (is_preserved == true)
                { // append to the candidate results
                    best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                }

                // check if the results are improved
                bool is_improved = (max_weighted_load_pm < min_max_weighted_load_pm || (max_weighted_load_pm == min_max_weighted_load_pm && bw_pm < min_bw_pm));
                if (is_improved == true)
                {
                    min_max_weighted_load_pm = max_weighted_load_pm;
                    min_bw_pm = bw_pm;

                    // clear previous results and append the new one
                    best_pm_nodes.clear();
                    best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                }
            }

            // randomly choose a best parity compute node
            size_t random_pos = Utils::randomUInt(0, best_pm_nodes.size() - 1, random_generator);
            uint32_t selected_pm_node = best_pm_nodes[random_pos].first;
            LoadTable &selected_cur_lt_after_pm = best_pm_nodes[random_pos].second;

            // update metadata
            selected_pm_nodes[parity_id] = selected_pm_node; // choose to compute parity block at the node
            cur_lt = selected_cur_lt_after_pm;               // update the current load table

            // subtract the bandwidth to retrieve original parity block
            uint8_t num_parities_stored_pm_node = stripe_group.parity_dists[parity_id][selected_pm_node];

            cur_lt.bw -= num_parities_stored_pm_node;                                 // subtract retrieving original parity block bandwidth
            cur_lt.bw += (min_bw_pm - (code.lambda_i - num_parities_stored_pm_node)); // add new parity block re-distribution bandwidth
            // printf("%u, %u\n", num_parities_stored_pm_node, min_bw_pm - num_parities_stored_pm_node);

            cur_block_placement[selected_pm_node]++; // place the computed parity block at the node

            // printf("stripe group: %u, compute parity %u at node %u, cur_lt: (max_load: %u, bw: %u, bw_sum: %u)\n", stripe_group.id, parity_id, selected_pm_node, min_max_load_pm, min_bw_pm, cur_lt.bw);
            // Utils::printVector(selected_cur_lt_after_pm.slt);
            // Utils::printVector(selected_cur_lt_after_pm.rlt);
        }

        // apply the load table for stripe group
        stripe_group.applied_lt = stripe_group.genPartialLTForParityCompute(EncodeMethod::PARITY_MERGE, selected_pm_nodes);
    }

    // weighted load table
    vector<double> weighted_slt(settings.num_nodes);
    vector<double> weighted_rlt(settings.num_nodes);
    for (uint16_t node_id = 0; node_id < settings.num_nodes; node_id++)
    {
        // weighted_load = load / bw
        weighted_slt[node_id] = 1.0 * cur_lt.slt[node_id] / settings.bw_profile.upload[node_id];
        weighted_rlt[node_id] = 1.0 * cur_lt.rlt[node_id] / settings.bw_profile.download[node_id];
    }

    gettimeofday(&end_time, nullptr);
    double finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                         (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("finished initialization for %lu stripe groups, time: %f ms\n", stripe_batch.selected_sgs.size(), finish_time);

    printf("find initial solution, weighted_lt:\n");
    printf("send load: ");
    Utils::printVector(weighted_slt);
    printf("recv load: ");
    Utils::printVector(weighted_rlt);

    printf("bandwidth: %u\n", cur_lt.bw);
}

void BART::initSolOfParityGenerationForPMData(StripeBatch &stripe_batch, vector<vector<bool>> &is_perfect_pm, LoadTable &cur_lt)
{
    // based on the send load from data relocation, for each parity block,
    // we identify the most load balanced encoding node for parity block
    // generation

    // metric for choosing load tables: (1) minimized max load; if (1)
    // finds multiple encoding nodes, randomly choose a node with minimum
    // bandwidth

    ConvertibleCode &code = stripe_batch.code;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    for (auto &item : stripe_batch.selected_sgs)
    {
        uint32_t sg_id = item.first;
        StripeGroup &stripe_group = item.second;

        // record nodes for parity merging
        u16string selected_pm_nodes(code.m_f, INVALID_NODE_ID);

        // record the block placement of each stripe group
        u16string cur_block_placement = stripe_group.data_dist;

        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            // skip parity blocks that can be perfectly merged
            if (is_perfect_pm[sg_id][parity_id] == true)
            {
                selected_pm_nodes[parity_id] = stripe_group.applied_lt.enc_nodes[parity_id];
                continue;
            }

            uint32_t min_max_load_pm = UINT32_MAX;
            uint8_t min_bw_pm = UINT8_MAX;
            vector<pair<uint16_t, LoadTable>> best_pm_nodes;

            for (uint16_t cand_pm_node = 0; cand_pm_node < num_nodes; cand_pm_node++)
            {
                // load table after applying the pm node
                LoadTable cur_lt_after_pm = cur_lt;

                uint8_t num_parities_stored_pm_node = stripe_group.parity_dists[parity_id][cand_pm_node];

                // bandwidth for the current solution
                uint8_t bw_pm = code.lambda_i - num_parities_stored_pm_node;

                // update the send load table
                // add the parity blocks to send
                for (auto &stripe : stripe_group.pre_stripes)
                {
                    uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                    if (parity_stored_node != cand_pm_node)
                    {
                        cur_lt_after_pm.slt[parity_stored_node]++;
                    }
                }

                // check if parity block relocation is needed
                if (cur_block_placement[cand_pm_node] > 0)
                {
                    cur_lt_after_pm.slt[cand_pm_node]++;
                    bw_pm++;
                }

                // update the recv load table
                cur_lt_after_pm.rlt[cand_pm_node] += (code.lambda_i - num_parities_stored_pm_node);

                // maximum load for the current solution
                uint32_t max_send_load_pm = *max_element(cur_lt_after_pm.slt.begin(), cur_lt_after_pm.slt.end());
                uint32_t max_recv_load_pm = *max_element(cur_lt_after_pm.rlt.begin(), cur_lt_after_pm.rlt.end());

                // maximum load and bandwidth
                uint32_t max_load_pm = max(max_send_load_pm, max_recv_load_pm);

                // check if the results are preserved
                bool is_preserved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm <= min_bw_pm));
                if (is_preserved == true)
                { // append to the candidate results
                    best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                }

                // check if the results are improved
                bool is_improved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm < min_bw_pm));
                if (is_improved == true)
                {
                    min_max_load_pm = max_load_pm;
                    min_bw_pm = bw_pm;

                    // clear previous results and append the new one
                    best_pm_nodes.clear();
                    best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                }
            }

            // randomly choose a best parity compute node
            size_t random_pos = Utils::randomUInt(0, best_pm_nodes.size() - 1, random_generator);
            uint32_t selected_pm_node = best_pm_nodes[random_pos].first;
            LoadTable &selected_cur_lt_after_pm = best_pm_nodes[random_pos].second;

            // update metadata
            selected_pm_nodes[parity_id] = selected_pm_node; // choose to compute parity block at the node
            cur_lt = selected_cur_lt_after_pm;               // update the current load table
            cur_lt.bw += min_bw_pm;                          // add bandwidth
            cur_block_placement[selected_pm_node]++;         // place the computed parity block at the node

            // printf("stripe group: %u, compute parity %u at node %u, cur_lt: (max_load: %u, bw: %u, bw_sum: %u)\n", stripe_group.id, parity_id, selected_pm_node, min_max_load_pm, min_bw_pm, cur_lt.bw);
            // Utils::printVector(selected_cur_lt_after_pm.slt);
            // Utils::printVector(selected_cur_lt_after_pm.rlt);
        }

        // apply the load table for stripe group
        stripe_group.applied_lt = stripe_group.genPartialLTForParityCompute(EncodeMethod::PARITY_MERGE, selected_pm_nodes);
    }

    printf("find initial solution, cur_lt:\n");
    printf("send load: ");
    Utils::printVector(cur_lt.slt);
    printf("recv load: ");
    Utils::printVector(cur_lt.rlt);
    printf("bandwidth: %u\n", cur_lt.bw);
}

void BART::optimizeSolOfParityGenerationForPM(StripeBatch &stripe_batch, vector<vector<bool>> &is_perfect_pm, LoadTable &cur_lt)
{
    // based on the previous solution, we adjust the encoding nodes to see
    // whether we can optimize the solution

    // metric for choosing load tables: (1) minimized max load; if (1)
    // finds multiple encoding nodes, randomly choose a node with minimum
    // bandwidth

    ConvertibleCode &code = stripe_batch.code;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    printf("start optimization of parity block generation\n");

    // use a while loop until the solution cannot be further optimized (i.e., cannot further improve load balance and then bandwidth)
    uint32_t max_send_load_iter = *max_element(cur_lt.slt.begin(), cur_lt.slt.end());
    uint32_t max_recv_load_iter = *max_element(cur_lt.rlt.begin(), cur_lt.rlt.end());
    uint32_t max_load_iter = max(max_send_load_iter, max_recv_load_iter);
    uint32_t bw_iter = cur_lt.bw;
    uint64_t iter = 0;
    while (true)
    {
        struct timeval start_time, end_time;
        gettimeofday(&start_time, nullptr);

        printf("start iteration %ld, cur_lt: (max_load: %u, bw: %u)\n", iter, max_load_iter, bw_iter);
        for (auto &item : stripe_batch.selected_sgs)
        {
            uint32_t sg_id = item.first;
            StripeGroup &stripe_group = item.second;

            // record the current block placement of each stripe group
            u16string cur_block_placement = stripe_group.data_dist;
            for (auto pm_node : stripe_group.applied_lt.enc_nodes)
            {
                cur_block_placement[pm_node]++;
            }

            bool is_sg_updated = false;

            for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
            {
                // skip parity blocks that can be perfectly merged
                if (is_perfect_pm[sg_id][parity_id] == true)
                {
                    continue;
                }

                // current pm node
                uint16_t cur_pm_node = stripe_group.applied_lt.enc_nodes[parity_id];

                // load table with removed load for current pm node
                LoadTable cur_lt_rm = cur_lt;

                // subtract the load for current pm node
                uint8_t num_parities_stored_cur_pm_node = stripe_group.parity_dists[parity_id][cur_pm_node];

                // original max load and bandwidth for the current solution
                uint32_t cur_max_send_load_pm = *max_element(cur_lt.slt.begin(), cur_lt.slt.end());
                uint32_t cur_max_recv_load_pm = *max_element(cur_lt.rlt.begin(), cur_lt.rlt.end());
                uint32_t cur_max_load_pm = max(cur_max_send_load_pm, cur_max_recv_load_pm);
                uint8_t cur_bw_pm = code.lambda_i - num_parities_stored_cur_pm_node;

                // update the send load table
                // subtract the parity blocks to send
                for (auto &stripe : stripe_group.pre_stripes)
                {
                    uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                    if (parity_stored_node != cur_pm_node)
                    {
                        cur_lt_rm.slt[parity_stored_node]--;
                    }
                }

                // check if parity block relocation is needed
                // subtract back the relocation bw
                if (cur_block_placement[cur_pm_node] > 1)
                {
                    // mark the block as not relocated
                    cur_block_placement[cur_pm_node]--;
                    cur_lt_rm.slt[cur_pm_node]--;
                    cur_bw_pm++;
                }

                // update the recv load table
                // subtract the parity blocks received
                cur_lt_rm.rlt[cur_pm_node] -= (code.lambda_i - num_parities_stored_cur_pm_node);

                // set min_max_load and bw as of before optimization
                uint32_t min_max_load_pm = cur_max_load_pm;
                uint8_t min_bw_pm = cur_bw_pm;

                // double min_stddev_pm = UINT_MAX;

                vector<pair<uint16_t, LoadTable>> best_pm_nodes;

                for (uint16_t cand_pm_node = 0; cand_pm_node < num_nodes; cand_pm_node++)
                {
                    // load table after applying the pm node
                    LoadTable cur_lt_after_pm = cur_lt_rm;

                    uint8_t num_parities_stored_pm_node = stripe_group.parity_dists[parity_id][cand_pm_node];

                    // bandwidth for the current solution
                    uint8_t bw_pm = code.lambda_i - num_parities_stored_pm_node;

                    // update the send load table
                    // add the parity blocks to send
                    for (auto &stripe : stripe_group.pre_stripes)
                    {
                        uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                        if (parity_stored_node != cand_pm_node)
                        {
                            cur_lt_after_pm.slt[parity_stored_node]++;
                        }
                    }

                    // check if parity block relocation is needed
                    if (cur_block_placement[cand_pm_node] > 0)
                    {
                        cur_lt_after_pm.slt[cand_pm_node]++;
                        bw_pm++;
                    }

                    // update the recv load table
                    cur_lt_after_pm.rlt[cand_pm_node] += (code.lambda_i - num_parities_stored_pm_node);

                    // maximum load for the current solution
                    uint32_t max_send_load_pm = *max_element(cur_lt_after_pm.slt.begin(), cur_lt_after_pm.slt.end());
                    uint32_t max_recv_load_pm = *max_element(cur_lt_after_pm.rlt.begin(), cur_lt_after_pm.rlt.end());

                    // maximum load and bandwidth
                    uint32_t max_load_pm = max(max_send_load_pm, max_recv_load_pm);

                    //*****************consider stddev**********************//
                    // // mean, stddev, cv (send)
                    // double mean_load_send_pm = 1.0 * std::accumulate(cur_lt_after_pm.slt.begin(), cur_lt_after_pm.slt.end(), 0) / cur_lt_after_pm.slt.size();
                    // double sqr_send = 0;
                    // for (auto &item : cur_lt_after_pm.slt)
                    // {
                    //     sqr_send += pow((double)item - mean_load_send_pm, 2);
                    // }
                    // double stddev_send = std::sqrt(sqr_send / cur_lt_after_pm.slt.size());

                    // // mean, stddev, cv (recv)
                    // double mean_load_recv_pm = 1.0 * std::accumulate(cur_lt_after_pm.rlt.begin(), cur_lt_after_pm.rlt.end(), 0) / cur_lt_after_pm.rlt.size();
                    // double sqr_recv = 0;
                    // for (auto &item : cur_lt_after_pm.rlt)
                    // {
                    //     sqr_recv += pow((double)item - mean_load_recv_pm, 2);
                    // }
                    // double stddev_recv = std::sqrt(sqr_recv / cur_lt_after_pm.rlt.size());

                    // // max stddev
                    // double stddev_max = max(stddev_send, stddev_recv);
                    //*********************************************//

                    // check if the results are preserved
                    bool is_preserved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm <= min_bw_pm));
                    // bool is_preserved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm < min_bw_pm) || (max_load_pm == min_max_load_pm && bw_pm == min_bw_pm && stddev_max <= min_stddev_pm));
                    if (is_preserved == true)
                    { // append to the candidate results
                        if (cand_pm_node != cur_pm_node)
                        { // check if we can find a different node
                            best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                        }
                    }

                    // check if the results are improved
                    bool is_improved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm < min_bw_pm));
                    // bool is_improved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm < min_bw_pm) || (max_load_pm == min_max_load_pm && bw_pm == min_bw_pm && stddev_max < min_stddev_pm));
                    if (is_improved == true)
                    {
                        min_max_load_pm = max_load_pm;
                        min_bw_pm = bw_pm;

                        // min_stddev_pm = stddev_max;

                        // clear previous results and append the new one
                        best_pm_nodes.clear();
                        best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                    }
                }

                unsigned int num_solutions = best_pm_nodes.size();
                if (num_solutions == 0)
                { // we cannot identify such a node
                  // mark the block as relocated again
                    cur_block_placement[cur_pm_node]++;
                }
                else
                { // we can identify such a node
                    // randomly choose a best parity compute node
                    size_t random_pos = Utils::randomUInt(0, best_pm_nodes.size() - 1, random_generator);
                    uint32_t selected_pm_node = best_pm_nodes[random_pos].first;
                    LoadTable &selected_cur_lt_after_pm = best_pm_nodes[random_pos].second;

                    // update metadata
                    is_sg_updated = true;
                    stripe_group.applied_lt.enc_nodes[parity_id] = selected_pm_node; // choose to compute parity block at the node
                    cur_lt = selected_cur_lt_after_pm;                               // update the current load table
                    cur_lt.bw = cur_lt.bw - cur_bw_pm + min_bw_pm;                   // update the bandwidth
                    cur_block_placement[selected_pm_node]++;                         // place the computed parity block at the node

                    // printf("stripe group: %u, change for parity %u: (%u -> %u), original_cur_lt: (max_load: %u, bw: %u) new_cur_lt: (max_load: %u, bw: %u), bw_sum: %u\n", stripe_group.id, parity_id, cur_pm_node, selected_pm_node, cur_max_load_pm, cur_bw_pm, min_max_load_pm, min_bw_pm, cur_lt.bw);
                    // Utils::printVector(selected_cur_lt_after_pm.slt);
                    // Utils::printVector(selected_cur_lt_after_pm.rlt);
                }
            }

            if (is_sg_updated == true)
            {
                // apply the load table for stripe group with the latest
                // encoding nodes
                stripe_group.applied_lt = stripe_group.genPartialLTForParityCompute(EncodeMethod::PARITY_MERGE, stripe_group.applied_lt.enc_nodes);
            }
        }

        // summarize the max load and bw after the current iteration
        uint32_t max_send_load_after_opt = *max_element(cur_lt.slt.begin(), cur_lt.slt.end());
        uint32_t max_recv_load_after_opt = *max_element(cur_lt.rlt.begin(), cur_lt.rlt.end());
        uint32_t max_load_after_opt = max(max_send_load_after_opt, max_recv_load_after_opt);
        uint32_t bw_after_opt = cur_lt.bw;
        bool improved = max_load_after_opt < max_load_iter || (max_load_after_opt == max_load_iter && bw_after_opt < bw_iter);

        printf("end iteration: %ld, cur_lt: (max_load: %u, bw: %u), improved: %u\n", iter, max_load_after_opt, bw_after_opt, improved);
        printf("send load: ");
        Utils::printVector(cur_lt.slt);
        printf("recv load: ");
        Utils::printVector(cur_lt.rlt);
        printf("bandwidth: %u\n", cur_lt.bw);

        gettimeofday(&end_time, nullptr);
        double finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                             (end_time.tv_usec - start_time.tv_usec) / 1000;
        printf("finished running the iteration %lu for %lu stripe groups, time: %f ms\n", iter, stripe_batch.selected_sgs.size(), finish_time);

        if (improved == true)
        {
            // update the max load and bw
            max_load_iter = max_load_after_opt;
            bw_iter = bw_after_opt;
            iter++;
        }
        else
        {
            // the solution cannot be further optimized, break the loop
            break;
        }
    }

    printf("finished optimization of parity block generation\n");
}

void BART::optimizeWeightedSolOfParityGenerationForPM(StripeBatch &stripe_batch, vector<vector<bool>> &is_perfect_pm, LoadTable &cur_lt)
{
    // based on the previous solution, we adjust the encoding nodes to see
    // whether we can optimize the solution

    // weighted version: weight_val = (# of blocks transferred / bw)

    // metric for choosing load tables: (1) minimized max load; if (1)
    // finds multiple encoding nodes, randomly choose a node with minimum
    // bandwidth

    ConvertibleCode &code = stripe_batch.code;
    ClusterSettings &settings = stripe_batch.settings;
    uint16_t num_nodes = stripe_batch.settings.num_nodes;

    printf("start optimization of parity block generation\n");

    // use a while loop until the solution cannot be further optimized (i.e.,
    // cannot further improve load balance and then bandwidth)

    // compute maximum weighted load for the iteration
    double max_weighted_load_iter = 0;
    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        // weighted_load = load / bw
        double weighted_send_load = 1.0 * cur_lt.slt[node_id] / settings.bw_profile.upload[node_id];
        double weighted_recv_load = 1.0 * cur_lt.rlt[node_id] / settings.bw_profile.download[node_id];
        max_weighted_load_iter = max({max_weighted_load_iter, weighted_send_load, weighted_recv_load});
    }

    uint32_t bw_iter = cur_lt.bw;
    uint64_t iter = 0;
    while (true)
    {
        struct timeval start_time, end_time;
        gettimeofday(&start_time, nullptr);

        printf("start iteration %ld, cur_lt: (max_load: %f, bw: %u)\n", iter, max_weighted_load_iter, bw_iter);
        // printf("start iteration %ld, cur_lt: (max_load: %u, bw: %u)\n", iter, max_load_iter, bw_iter);
        for (auto &item : stripe_batch.selected_sgs)
        {
            uint32_t sg_id = item.first;
            StripeGroup &stripe_group = item.second;

            // record the current block placement of each stripe group
            u16string cur_block_placement = stripe_group.data_dist;
            for (auto pm_node : stripe_group.applied_lt.enc_nodes)
            {
                cur_block_placement[pm_node]++;
            }

            bool is_sg_updated = false;

            for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
            {
                // skip parity blocks that can be perfectly merged
                if (is_perfect_pm[sg_id][parity_id] == true)
                {
                    continue;
                }

                // current pm node
                uint16_t cur_pm_node = stripe_group.applied_lt.enc_nodes[parity_id];

                // load table with removed load for current pm node
                LoadTable cur_lt_rm = cur_lt;

                // subtract the load for current pm node
                uint8_t num_parities_stored_cur_pm_node = stripe_group.parity_dists[parity_id][cur_pm_node];

                // compute original max weighted load and bandwidth for the current solution
                double cur_max_weighted_load_pm = 0;
                for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
                {
                    // weighted_load = load / bw
                    double weighted_send_load = 1.0 * cur_lt.slt[node_id] / settings.bw_profile.upload[node_id];
                    double weighted_recv_load = 1.0 * cur_lt.rlt[node_id] / settings.bw_profile.download[node_id];
                    cur_max_weighted_load_pm = max({cur_max_weighted_load_pm, weighted_send_load, weighted_recv_load});
                }

                uint8_t cur_bw_pm = code.lambda_i - num_parities_stored_cur_pm_node;

                // update the send load table
                // subtract the parity blocks to send
                for (auto &stripe : stripe_group.pre_stripes)
                {
                    uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                    if (parity_stored_node != cur_pm_node)
                    {
                        cur_lt_rm.slt[parity_stored_node]--;
                    }
                }

                // check if parity block relocation is needed
                // subtract back the relocation bw
                if (cur_block_placement[cur_pm_node] > 1)
                {
                    // mark the block as not relocated
                    cur_block_placement[cur_pm_node]--;
                    cur_lt_rm.slt[cur_pm_node]--;
                    cur_bw_pm++;
                }

                // update the recv load table
                // subtract the parity blocks received
                cur_lt_rm.rlt[cur_pm_node] -= (code.lambda_i - num_parities_stored_cur_pm_node);

                // set min_max_weighted_load and bw as of before optimization
                double min_max_weighted_load_pm = cur_max_weighted_load_pm;
                uint8_t min_bw_pm = cur_bw_pm;

                // double min_stddev_pm = UINT_MAX;

                vector<pair<uint16_t, LoadTable>> best_pm_nodes;

                for (uint16_t cand_pm_node = 0; cand_pm_node < num_nodes; cand_pm_node++)
                {
                    // load table after applying the pm node
                    LoadTable cur_lt_after_pm = cur_lt_rm;

                    uint8_t num_parities_stored_pm_node = stripe_group.parity_dists[parity_id][cand_pm_node];

                    // bandwidth for the current solution
                    uint8_t bw_pm = code.lambda_i - num_parities_stored_pm_node;

                    // update the send load table
                    // add the parity blocks to send
                    for (auto &stripe : stripe_group.pre_stripes)
                    {
                        uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                        if (parity_stored_node != cand_pm_node)
                        {
                            cur_lt_after_pm.slt[parity_stored_node]++;
                        }
                    }

                    // check if parity block relocation is needed
                    if (cur_block_placement[cand_pm_node] > 0)
                    {
                        cur_lt_after_pm.slt[cand_pm_node]++;
                        bw_pm++;
                    }

                    // update the recv load table
                    cur_lt_after_pm.rlt[cand_pm_node] += (code.lambda_i - num_parities_stored_pm_node);

                    // compute maximum weighted load for the current solution
                    double max_weighted_load_pm = 0;
                    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
                    {
                        // weighted_load = load / bw
                        double weighted_send_load = 1.0 * cur_lt_after_pm.slt[node_id] / settings.bw_profile.upload[node_id];
                        double weighted_recv_load = 1.0 * cur_lt_after_pm.rlt[node_id] / settings.bw_profile.download[node_id];
                        max_weighted_load_pm = max({max_weighted_load_pm, weighted_send_load, weighted_recv_load});
                    }

                    //*****************consider stddev**********************//
                    // // mean, stddev, cv (send)
                    // double mean_load_send_pm = 1.0 * std::accumulate(cur_lt_after_pm.slt.begin(), cur_lt_after_pm.slt.end(), 0) / cur_lt_after_pm.slt.size();
                    // double sqr_send = 0;
                    // for (auto &item : cur_lt_after_pm.slt)
                    // {
                    //     sqr_send += pow((double)item - mean_load_send_pm, 2);
                    // }
                    // double stddev_send = std::sqrt(sqr_send / cur_lt_after_pm.slt.size());

                    // // mean, stddev, cv (recv)
                    // double mean_load_recv_pm = 1.0 * std::accumulate(cur_lt_after_pm.rlt.begin(), cur_lt_after_pm.rlt.end(), 0) / cur_lt_after_pm.rlt.size();
                    // double sqr_recv = 0;
                    // for (auto &item : cur_lt_after_pm.rlt)
                    // {
                    //     sqr_recv += pow((double)item - mean_load_recv_pm, 2);
                    // }
                    // double stddev_recv = std::sqrt(sqr_recv / cur_lt_after_pm.rlt.size());

                    // // max stddev
                    // double stddev_max = max(stddev_send, stddev_recv);
                    //*********************************************//

                    // check if the results are preserved
                    bool is_preserved = (max_weighted_load_pm < min_max_weighted_load_pm || (max_weighted_load_pm == min_max_weighted_load_pm && bw_pm <= min_bw_pm));
                    // bool is_preserved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm < min_bw_pm) || (max_load_pm == min_max_load_pm && bw_pm == min_bw_pm && stddev_max <= min_stddev_pm));
                    if (is_preserved == true)
                    { // append to the candidate results
                        if (cand_pm_node != cur_pm_node)
                        { // check if we can find a different node
                            best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                        }
                    }

                    // check if the results are improved
                    bool is_improved = (max_weighted_load_pm < min_max_weighted_load_pm || (max_weighted_load_pm == min_max_weighted_load_pm && bw_pm < min_bw_pm));
                    // bool is_improved = (max_load_pm < min_max_load_pm || (max_load_pm == min_max_load_pm && bw_pm < min_bw_pm) || (max_load_pm == min_max_load_pm && bw_pm == min_bw_pm && stddev_max < min_stddev_pm));
                    if (is_improved == true)
                    {
                        min_max_weighted_load_pm = max_weighted_load_pm;
                        min_bw_pm = bw_pm;

                        // min_stddev_pm = stddev_max;

                        // clear previous results and append the new one
                        best_pm_nodes.clear();
                        best_pm_nodes.push_back(pair<uint16_t, LoadTable>(cand_pm_node, cur_lt_after_pm));
                    }
                }

                unsigned int num_solutions = best_pm_nodes.size();
                if (num_solutions == 0)
                { // we cannot identify such a node
                  // mark the block as relocated again
                    cur_block_placement[cur_pm_node]++;
                }
                else
                { // we can identify such a node
                    // randomly choose a best parity compute node
                    size_t random_pos = Utils::randomUInt(0, best_pm_nodes.size() - 1, random_generator);
                    uint32_t selected_pm_node = best_pm_nodes[random_pos].first;
                    LoadTable &selected_cur_lt_after_pm = best_pm_nodes[random_pos].second;

                    // update metadata
                    is_sg_updated = true;
                    stripe_group.applied_lt.enc_nodes[parity_id] = selected_pm_node; // choose to compute parity block at the node
                    cur_lt = selected_cur_lt_after_pm;                               // update the current load table
                    cur_lt.bw = cur_lt.bw - cur_bw_pm + min_bw_pm;                   // update the bandwidth
                    cur_block_placement[selected_pm_node]++;                         // place the computed parity block at the node

                    // printf("stripe group: %u, change for parity %u: (%u -> %u), original_cur_lt: (max_load: %u, bw: %u) new_cur_lt: (max_load: %u, bw: %u), bw_sum: %u\n", stripe_group.id, parity_id, cur_pm_node, selected_pm_node, cur_max_load_pm, cur_bw_pm, min_max_load_pm, min_bw_pm, cur_lt.bw);
                    // Utils::printVector(selected_cur_lt_after_pm.slt);
                    // Utils::printVector(selected_cur_lt_after_pm.rlt);
                }
            }

            if (is_sg_updated == true)
            {
                // apply the load table for stripe group with the latest
                // encoding nodes
                stripe_group.applied_lt = stripe_group.genPartialLTForParityCompute(EncodeMethod::PARITY_MERGE, stripe_group.applied_lt.enc_nodes);
            }
        }

        // summarize the max weighted load and bw after the current iteration
        double max_weighted_load_after_opt = 0;
        for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
        {
            // weighted_load = load / bw
            double weighted_send_load = 1.0 * cur_lt.slt[node_id] / settings.bw_profile.upload[node_id];
            double weighted_recv_load = 1.0 * cur_lt.rlt[node_id] / settings.bw_profile.download[node_id];
            max_weighted_load_after_opt = max({max_weighted_load_after_opt, weighted_send_load, weighted_recv_load});
        }

        uint32_t bw_after_opt = cur_lt.bw;

        bool improved = max_weighted_load_after_opt < max_weighted_load_iter || (max_weighted_load_after_opt == max_weighted_load_iter && bw_after_opt < bw_iter);

        printf("end iteration: %ld, cur_lt: (max_load: %f, bw: %u), improved: %u\n", iter, max_weighted_load_after_opt, bw_after_opt, improved);

        // weighted load table
        vector<double> weighted_slt(settings.num_nodes);
        vector<double> weighted_rlt(settings.num_nodes);
        for (uint16_t node_id = 0; node_id < settings.num_nodes; node_id++)
        {
            // weighted_load = load / bw
            weighted_slt[node_id] = 1.0 * cur_lt.slt[node_id] / settings.bw_profile.upload[node_id];
            weighted_rlt[node_id] = 1.0 * cur_lt.rlt[node_id] / settings.bw_profile.download[node_id];
        }

        printf("optimized weighted_lt:\n");
        printf("send load: ");
        Utils::printVector(weighted_slt);
        printf("recv load: ");
        Utils::printVector(weighted_rlt);

        printf("bandwidth: %u\n", cur_lt.bw);

        gettimeofday(&end_time, nullptr);
        double finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                             (end_time.tv_usec - start_time.tv_usec) / 1000;
        printf("finished running the iteration %lu for %lu stripe groups, time: %f ms\n", iter, stripe_batch.selected_sgs.size(), finish_time);

        if (improved == true)
        {
            // update the max load and bw
            max_weighted_load_iter = max_weighted_load_after_opt;
            bw_iter = bw_after_opt;
            iter++;
        }
        else
        {
            // the solution cannot be further optimized, break the loop
            break;
        }
    }

    printf("finished optimization of parity block generation\n");
}

void BART::genParityComputationHybrid(StripeBatch &stripe_batch, string approach)
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
            if (stripe_group.isPerfectPM() == true)
            {
                // generate parity computation scheme for parity merging
                stripe_group.genLTForPerfectPM();
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

            // based on the send load from data relocation, for each stripe group, generate all possible solutions for parity computation (for re-encoding and/or parity merging) and finds the most load-balanced solution (which determines the parity generation method and node)

            // metric for choosing load tables: (1) minimize max load: max(max(cur_lt.slt + slt), max(cur_lt.rlt + rlt)); if (1) finds multiple load tables, choose the load table with minimum bandwidth; if (2) also finds multiple lts, randomly pick one

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
                            u16string enc_nodes(code.m_f, encode_node_id);
                            LoadTable cand_lt = stripe_group.genPartialLTForParityCompute(EncodeMethod::RE_ENCODE, enc_nodes);
                            best_lts.push_back(move(cand_lt));
                        }
                    }
                }
            }

            if (approach == "BTPM" || approach == "BT")
            { // parity merging

                // For the brute-force method, we need to enumerate all possible parity block compute nodes (M ^ code.m_f solutions);
                // we can greedily choosing the best solution for each parity block to reduce the computation overhead (M * code.m_f solutions)
                u16string pm_nodes(code.m_f, INVALID_NODE_ID); // computation for parity i is at pm_nodes[i]

                // record cur_lt_after after applying each parity block computation; record generated bandwidth
                LoadTable cur_lt_after_pm = cur_lt_after;
                uint8_t cur_pm_bw = 0;
                // record current block placement for selected parities
                u16string cur_block_placement = stripe_group.data_dist;

                for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                {
                    uint32_t min_max_load_pm_parity = UINT32_MAX;
                    uint8_t min_bw_pm_parity = UINT8_MAX;
                    vector<pair<uint16_t, LoadTable>> best_parity_compute_nodes;

                    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
                    {
                        uint16_t parity_compute_node = node_id;
                        uint8_t num_stored_parity_cmp_node = stripe_group.parity_dists[parity_id][parity_compute_node];

                        // record cur_lt_after_pm_parity after compute the parity block at node_id
                        LoadTable cur_lt_after_pm_parity = cur_lt_after_pm;

                        // bandwidth for the current solution
                        uint8_t bw_pm_parity = code.lambda_i - num_stored_parity_cmp_node;

                        // update the send load table
                        // add the parity blocks to send
                        for (auto &stripe : stripe_group.pre_stripes)
                        {
                            uint16_t parity_stored_node = stripe->indices[code.k_i + parity_id];
                            if (parity_stored_node != parity_compute_node)
                            {
                                cur_lt_after_pm_parity.slt[parity_stored_node]++;
                            }
                        }

                        if (cur_block_placement[parity_compute_node] > 0)
                        { // check if parity block relocation is needed
                            cur_lt_after_pm_parity.slt[parity_compute_node]++;
                            bw_pm_parity++;
                        }

                        // update the recv load table
                        cur_lt_after_pm_parity.rlt[parity_compute_node] += (code.lambda_i - num_stored_parity_cmp_node);

                        // maximum load for the current solution
                        uint32_t max_send_load_pm_parity = *max_element(cur_lt_after_pm_parity.slt.begin(), cur_lt_after_pm_parity.slt.end());
                        uint32_t max_recv_load_pm_parity = *max_element(cur_lt_after_pm_parity.rlt.begin(), cur_lt_after_pm_parity.rlt.end());

                        // maximum load and bandwidth
                        uint32_t max_load_pm_parity = max(max_send_load_pm_parity, max_recv_load_pm_parity);

                        // update min_max_load and min_bw
                        if (max_load_pm_parity <= min_max_load_pm_parity)
                        {
                            // have improved result
                            if (max_load_pm_parity < min_max_load_pm_parity || (max_load_pm_parity == min_max_load_pm_parity && bw_pm_parity < min_bw_pm_parity))
                            {
                                min_max_load_pm_parity = max_load_pm_parity;
                                min_bw_pm_parity = bw_pm_parity;
                                best_parity_compute_nodes.clear();
                            }
                            if (bw_pm_parity <= min_bw_pm_parity)
                            {
                                best_parity_compute_nodes.push_back(pair<uint16_t, LoadTable>(parity_compute_node, cur_lt_after_pm_parity));
                            }
                        }
                    }

                    // randomly choose a best parity compute node
                    size_t random_pos = Utils::randomUInt(0, best_parity_compute_nodes.size() - 1, random_generator);
                    uint32_t selected_parity_compute_node = best_parity_compute_nodes[random_pos].first;
                    LoadTable &selected_cur_lt_after_pm_parity = best_parity_compute_nodes[random_pos].second;

                    // update metadata
                    pm_nodes[parity_id] = selected_parity_compute_node;  // choose to compute parity block at the node
                    cur_lt_after_pm = selected_cur_lt_after_pm_parity;   // update the current load table
                    cur_pm_bw += min_bw_pm_parity;                       // add bandwidth
                    cur_block_placement[selected_parity_compute_node]++; // place the computed parity block at the node

                    // printf("stripe group: %u, choose to compute parity %u at node %u, bandwidth: %u\n", stripe_group.id, parity_id, selected_parity_compute_node, min_bw_pm_parity);
                    // Utils::printVector(cur_lt_after_pm.slt);
                    // Utils::printVector(cur_lt_after_pm.rlt);
                }

                // maximum load for the pm solution (after choosing the computation nodes for all parity blocks)
                uint32_t max_send_load_pm = *max_element(cur_lt_after_pm.slt.begin(), cur_lt_after_pm.slt.end());
                uint32_t max_recv_load_pm = *max_element(cur_lt_after_pm.rlt.begin(), cur_lt_after_pm.rlt.end());
                uint32_t max_load = max(max_send_load_pm, max_recv_load_pm);

                // update min_max_load and min_bw
                if (max_load <= min_max_load)
                {
                    // have improved result
                    if (max_load < min_max_load || (max_load == min_max_load && cur_pm_bw < min_bw))
                    {
                        min_max_load = max_load;
                        min_bw = cur_pm_bw;
                        best_lts.clear();
                    }
                    if (cur_pm_bw <= min_bw)
                    {
                        LoadTable cand_lt = stripe_group.genPartialLTForParityCompute(EncodeMethod::PARITY_MERGE, pm_nodes);
                        best_lts.push_back(move(cand_lt));
                    }
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

void BART::genStripeRedistribution(StripeBatch &stripe_batch)
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

void BART::genWeightedStripeRedistribution(StripeBatch &stripe_batch)
{
    // NOTE: before this step, the solution of parity computation (encoding
    // methods and nodes) are fixed; in this step, for each stripe group we
    // schedule the relocation of data and parity blocks by finding nodes to
    // place them

    // weighted version (weight = load / bw)

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

    // create rvtx to weight map, each item: <rvtx_id, node_weight (download bandwidth)>
    // weight equals the download bandwidth of the node
    unordered_map<uint64_t, double> rvtx2weight_map;
    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        uint64_t rvtx_id = node2rvtx_map[node_id];
        rvtx2weight_map[rvtx_id] = stripe_batch.settings.bw_profile.download[node_id];
    }

    // step 3: find optimal semi-matching (based on the initial receive load)
    vector<uint64_t>
        sm_edges = bipartite.findOptWeightedSemiMatching(lvtx2sg_map, sg2lvtx_map, rvtx2weight_map);

    printf("finished finding weighted semi-matching solutions\n");

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