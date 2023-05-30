#include "StripeBatch.hh"

StripeBatch::StripeBatch(uint8_t _id, ConvertibleCode &_code, ClusterSettings &_settings, mt19937 &_random_generator) : id(_id), code(_code), settings(_settings), random_generator(_random_generator)
{

    // init pre-transition stripes
    pre_stripes.clear();
    pre_stripes.assign(settings.num_stripes, Stripe());
    for (uint32_t stripe_id = 0; stripe_id < settings.num_stripes; stripe_id++)
    {
        pre_stripes[stripe_id].id = stripe_id;
        pre_stripes[stripe_id].indices.assign(code.n_i, INVALID_NODE_ID);
    }

    // init post-transition stripes
    uint32_t num_post_stripes = settings.num_stripes / code.lambda_i;
    post_stripes.clear();
    post_stripes.assign(num_post_stripes, Stripe());
    for (uint32_t post_stripe_id = 0; post_stripe_id < num_post_stripes; post_stripe_id++)
    {
        post_stripes[post_stripe_id].id = post_stripe_id;
        post_stripes[post_stripe_id].indices.assign(code.n_f, INVALID_NODE_ID);
    }
}

StripeBatch::~StripeBatch()
{
}

void StripeBatch::print()
{
    printf("StripeBatch %u:\n", id);
    for (auto &item : selected_sgs)
    {
        item.second.print();
    }
}

void StripeBatch::constructSGInSequence()
{
    uint32_t num_sgs = settings.num_stripes / code.lambda_i;

    selected_sgs.clear();

    // initialize sg_stripe_ids
    for (uint32_t sg_id = 0; sg_id < num_sgs; sg_id++)
    {
        u32string sg_stripe_ids(code.lambda_i, 0);
        vector<Stripe *> selected_pre_stripes(code.lambda_i, NULL);
        for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
        {
            uint32_t sb_stripe_id = sg_id * code.lambda_i + stripe_id;
            sg_stripe_ids[stripe_id] = sb_stripe_id;
            selected_pre_stripes[stripe_id] = &pre_stripes[sb_stripe_id];
        }
        selected_sgs.insert(pair<uint32_t, StripeGroup>(sg_id, StripeGroup(sg_id, code, settings, selected_pre_stripes, &post_stripes[sg_id])));
    }
}

void StripeBatch::constructSGByRandomPick()
{
    uint32_t num_sgs = settings.num_stripes / code.lambda_i;

    selected_sgs.clear();

    // shuffle indices of stripes
    vector<uint32_t> shuff_sb_stripe_idxs(pre_stripes.size(), 0);
    for (uint32_t stripe_id = 0; stripe_id < pre_stripes.size(); stripe_id++)
    {
        shuff_sb_stripe_idxs[stripe_id] = stripe_id;
    }

    shuffle(shuff_sb_stripe_idxs.begin(), shuff_sb_stripe_idxs.end(), random_generator);

    // initialize sg_stripe_ids
    for (uint32_t sg_id = 0; sg_id < num_sgs; sg_id++)
    {
        u32string sg_stripe_ids(code.lambda_i, 0);
        vector<Stripe *> selected_pre_stripes(code.lambda_i, NULL);
        for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
        {
            // get shuffled stripe index
            uint32_t sb_stripe_id = shuff_sb_stripe_idxs[sg_id * code.lambda_i + stripe_id];
            sg_stripe_ids[stripe_id] = sb_stripe_id;
            selected_pre_stripes[stripe_id] = &pre_stripes[sb_stripe_id];
        }
        selected_sgs.insert(pair<uint32_t, StripeGroup>(sg_id, StripeGroup(sg_id, code, settings, selected_pre_stripes, &post_stripes[sg_id])));
    }
}

void StripeBatch::constructSGByBW(string approach)
{
    uint32_t num_sgs = settings.num_stripes / code.lambda_i;

    selected_sgs.clear();

    // enumerate all possible stripe groups
    uint64_t num_combs = Utils::calCombSize(settings.num_stripes, code.lambda_i);

    uint64_t num_cand_sgs = num_combs;
    printf("candidates stripe groups: %ld\n", num_cand_sgs);

    /**
     * @brief bandwidth table for all stripe groups
     *  bandwidth 0: sg_0; sg_1; ...
     *  bandwidth 1: sg_2; sg_3; ...
     *  ...
     *  bandwidth n_f: ...
     */

    uint8_t max_bw = code.n_f + code.k_f;           // allowed maximum bandwidth
    vector<vector<u32string>> bw_sgs_table(max_bw); // record sg_stripe_ids for each candidate stripe group

    // current stripe ids
    u32string sg_stripe_ids(code.lambda_i, 0);
    for (uint32_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
    {
        sg_stripe_ids[stripe_id] = stripe_id;
    }

    // calculate transition bandwidth for each stripe group
    for (uint64_t cand_sg_id = 0; cand_sg_id < num_cand_sgs; cand_sg_id++)
    {
        // // get stripe ids
        // u32string sg_stripe_ids = Utils::getCombFromPosition(settings.num_stripes, code.lambda_i, cand_sg_id);

        vector<Stripe *> selected_pre_stripes(code.lambda_i, NULL);
        for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
        {
            selected_pre_stripes[stripe_id] = &pre_stripes[sg_stripe_ids[stripe_id]];
        }

        StripeGroup stripe_group(0, code, settings, selected_pre_stripes, NULL);
        uint8_t min_bw = stripe_group.getMinTransBW(approach);
        bw_sgs_table[min_bw].push_back(sg_stripe_ids);

        // printf("candidate stripe group: %lu, minimum bandwidth: %u\n", cand_sg_id, min_bw);
        // Utils::printVector(sg_stripe_ids);

        // get next combination
        Utils::getNextComb(settings.num_stripes, code.lambda_i, sg_stripe_ids);

        // logging
        if (cand_sg_id % (uint64_t)pow(10, 7) == 0)
        {
            printf("stripe groups (%lu / %lu) bandwidth calculated\n", cand_sg_id, num_cand_sgs);
        }
    }

    // printf("bw_sgs_table:\n");
    // for (uint8_t bw = 0; bw < max_bw; bw++)
    // {
    //     printf("bandwidth: %u, %lu candidate stripe groups\n", bw, bw_sgs_table[bw].size());
    // }

    vector<uint32_t> bw_num_selected_sgs(max_bw, 0);           // record bw for selected stripes
    vector<bool> stripe_selected(settings.num_stripes, false); // mark if stripe is selected

    uint32_t sg_id = 0;
    for (uint8_t bw = 0; bw < max_bw && sg_id < num_sgs; bw++)
    {
        for (uint64_t idx = 0; idx < bw_sgs_table[bw].size(); idx++)
        {
            u32string &sg_stripe_ids = bw_sgs_table[bw][idx];
            // auto cand_sg_id = bw_sgs_table[bw][idx];
            // u32string sg_stripe_ids = Utils::getCombFromPosition(settings.num_stripes, code.lambda_i, cand_sg_id);

            // check whether the stripe group is valid (TODO: check why the performance is bad here)
            bool is_cand_sg_valid = true;
            for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
            {
                is_cand_sg_valid = is_cand_sg_valid && !stripe_selected[sg_stripe_ids[stripe_id]];
            }

            if (is_cand_sg_valid == true)
            {
                // add the valid stripe group
                vector<Stripe *> selected_pre_stripes(code.lambda_i, NULL);
                for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
                {
                    selected_pre_stripes[stripe_id] = &pre_stripes[sg_stripe_ids[stripe_id]];
                }
                selected_sgs.insert(pair<uint32_t, StripeGroup>(sg_id, StripeGroup(sg_id, code, settings, selected_pre_stripes, &post_stripes[sg_id])));

                // mark the stripes as selected
                sg_id++;
                for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
                {
                    stripe_selected[sg_stripe_ids[stripe_id]] = true;
                }

                // update stats
                bw_num_selected_sgs[bw]++;

                // printf("(%u / %u) stripe groups selected, bandwidth: %u, stripes: ", num_selected_sgs, num_sgs, bw);
                // Utils::printVector(sg_stripe_ids);
            }
        }
    }

    uint32_t total_bw_selected_sgs = 0;
    for (uint8_t bw = 0; bw < max_bw; bw++)
    {
        total_bw_selected_sgs += bw_num_selected_sgs[bw] * bw;
    }

    printf("selected %u stripe groups, total bandwidth: %u\n", num_sgs, total_bw_selected_sgs);
    printf("bw_num_selected_sgs:\n");
    for (uint8_t bw = 0; bw < max_bw; bw++)
    {
        if (bw_num_selected_sgs[bw] > 0)
        {
            printf("bandwidth = %u: %u stripe groups\n", bw, bw_num_selected_sgs[bw]);
        }
    }
}

// bool StripeBatch::constructByCostAndSendLoad(vector<Stripe> &stripes, mt19937 &random_generator)
// {

//     size_t num_stripes = stripes.size();

//     // check if the number of stripes is a multiple of lambda_i
//     if (num_stripes % _code.lambda_i != 0)
//     {
//         printf("invalid parameters\n");
//         return false;
//     }

//     // maintain a send load table
//     size_t num_nodes = _settings.M;
//     vector<size_t> cur_slt(num_nodes, 0);

//     // enumerate all possible stripe groups
//     vector<size_t> comb_ids;
//     vector<vector<size_t>> combinations = Utils::getCombinations(num_stripes, _code.lambda_i);
//     size_t num_candidate_sgs = combinations.size();

//     vector<int> candidate_sgs_costs(num_candidate_sgs, -1); // mark down the minimum cost for each candidate stripe group

//     vector<vector<size_t>> overlapped_cand_sgs(num_stripes, vector<size_t>()); // <stripe_id, overlapped_stripe_groups>

//     // enumerate transition costs for all candidate stripe groups
//     for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++)
//     {
//         comb_ids.push_back(cand_id);

//         // construct candidate stripe group
//         vector<size_t> &comb = combinations[cand_id];

//         // record overlapped candidate stripe_groups
//         for (auto stripe_id : comb)
//         {
//             overlapped_cand_sgs[stripe_id].push_back(cand_id);
//         }

//         vector<Stripe *> stripes_in_group;
//         for (size_t sid = 0; sid < _code.lambda_i; sid++)
//         {
//             stripes_in_group.push_back(&stripes[comb[sid]]);
//         }

//         StripeGroup candidate_sg(cand_id, _code, _settings, stripes_in_group);

//         // get transition cost
//         int transition_cost = candidate_sg.getMinTransitionCost();
//         candidate_sgs_costs[cand_id] = transition_cost;
//     }

//     printf("total number of candidate stripe groups: %ld\n", num_candidate_sgs);
//     // for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++) {
//     //     printf("candidate_id: %ld, cost: %d\n", cand_id, candidate_sgs_costs[cand_id]);
//     //     Utils::printUIntVector(combinations[cand_id]);
//     // }

//     vector<size_t> sorted_stripe_ids; // store sorted stripe ids

//     // record currently valid candidates; initialize as all candidate sgs
//     vector<bool> is_cand_valid(num_candidate_sgs, true);
//     vector<size_t> cur_valid_cand_ids = comb_ids;
//     vector<int> cur_valid_cand_costs = candidate_sgs_costs;

//     size_t num_sgs = num_stripes / _code.lambda_i;
//     for (size_t sg_id = 0; sg_id < num_sgs; sg_id++)
//     {
//         // get remaining valid combinations
//         vector<size_t> valid_cand_ids;
//         vector<int> valid_cand_costs;

//         // filter out invalid combinations
//         for (size_t idx = 0; idx < cur_valid_cand_ids.size(); idx++)
//         {
//             size_t cand_id = cur_valid_cand_ids[idx];
//             if (is_cand_valid[cand_id] == true)
//             {
//                 valid_cand_ids.push_back(cand_id);
//                 valid_cand_costs.push_back(candidate_sgs_costs[cand_id]);
//             }
//         }

//         // filter out stripe groups with minimum transition cost
//         int min_cost = *min_element(valid_cand_costs.begin(), valid_cand_costs.end());

//         vector<size_t> min_cost_cand_ids;
//         for (size_t idx = 0; idx < valid_cand_ids.size(); idx++)
//         {
//             if (valid_cand_costs[idx] == min_cost)
//             {
//                 min_cost_cand_ids.push_back(valid_cand_ids[idx]);
//             }
//         }
//         // find stripe groups with <minimum load after connection> among all min cost candidates
//         vector<size_t> cand_min_cost_loads(min_cost_cand_ids.size(), 0);
//         vector<vector<size_t>> cand_min_cost_slt(min_cost_cand_ids.size(), vector<size_t>());

//         for (size_t idx = 0; idx < min_cost_cand_ids.size(); idx++)
//         {
//             size_t min_load_cand_id = min_cost_cand_ids[idx];
//             // construct candidate stripe group
//             vector<size_t> &comb = combinations[min_load_cand_id];

//             vector<Stripe *> stripes_in_group;
//             for (size_t sid = 0; sid < _code.lambda_i; sid++)
//             {
//                 stripes_in_group.push_back(&stripes[comb[sid]]);
//             }

//             StripeGroup candidate_sg(min_load_cand_id, _code, _settings, stripes_in_group);

//             vector<vector<size_t>> cand_send_load_tables = candidate_sg.getCandSendLoadTablesForMinTransCost(min_cost);

//             size_t best_min_max_load = SIZE_MAX;
//             vector<size_t> best_min_max_load_slt;
//             for (auto &send_load_table : cand_send_load_tables)
//             {
//                 vector<size_t> send_load_table_after_connection = cur_slt;
//                 for (size_t node_id = 0; node_id < num_nodes; node_id++)
//                 {
//                     send_load_table_after_connection[node_id] += send_load_table[node_id];
//                 }

//                 size_t slt_mml_after_conn = *max_element(send_load_table_after_connection.begin(), send_load_table_after_connection.end());

//                 if (slt_mml_after_conn < best_min_max_load)
//                 {
//                     best_min_max_load = slt_mml_after_conn;
//                     best_min_max_load_slt = send_load_table;
//                 }
//             }

//             cand_min_cost_loads[idx] = best_min_max_load;
//             cand_min_cost_slt[idx] = best_min_max_load_slt;
//         }

//         // from candidate combinations, find the min_max load increase
//         size_t min_cost_min_max_load_idx = distance(cand_min_cost_loads.begin(), min_element(cand_min_cost_loads.begin(), cand_min_cost_loads.end()));
//         size_t min_cost_min_load = cand_min_cost_loads[min_cost_min_max_load_idx];
//         size_t min_cost_min_max_load_cand_id = min_cost_cand_ids[min_cost_min_max_load_idx];
//         vector<size_t> min_cost_min_max_load_slt = cand_min_cost_slt[min_cost_min_max_load_idx];
//         vector<size_t> &min_cost_min_max_load_comb = combinations[min_cost_min_max_load_cand_id];

//         // update current load table
//         for (size_t node_id = 0; node_id < num_nodes; node_id++)
//         {
//             cur_slt[node_id] += min_cost_min_max_load_slt[node_id];
//         }

//         for (auto stripe_id : min_cost_min_max_load_comb)
//         {
//             // add stripes from the selected stripe group to sorted stripes
//             sorted_stripe_ids.push_back(stripe_id);

//             // mark overlapped candidate stripe groups as invalid
//             for (auto cand_sg_ids : overlapped_cand_sgs[stripe_id])
//             {
//                 is_cand_valid[cand_sg_ids] = false;
//             }
//         }

//         // update currently valid combinations
//         cur_valid_cand_ids = valid_cand_ids;
//         cur_valid_cand_costs = valid_cand_costs;

//         // summarize current pick
//         printf("pick candidate_id: %ld, cost: %d, min_cost_min_load: %ld, remaining number of candidates: (%ld / %ld), cur_slt:\n", min_cost_min_max_load_cand_id, min_cost, min_cost_min_load, cur_valid_cand_ids.size(), num_candidate_sgs);
//         Utils::printUIntVector(cur_slt);
//     }

//     // if (sorted_stripe_ids.size() != num_stripes) {
//     //     printf("error: invalid sorted_stripe_ids\n");
//     //     return false;
//     // }

//     // 2. put stripes by sorted stripe ids (by cost) into stripe groups
//     vector<Stripe *> stripes_in_group;
//     size_t sg_id = 0; // stripe group id
//     for (size_t idx = 0; idx < num_stripes; idx++)
//     {

//         size_t stripe_id = sorted_stripe_ids[idx];

//         // add the stripe into stripe group
//         Stripe &stripe = stripes[stripe_id];
//         stripes_in_group.push_back(&stripe);

//         if (stripes_in_group.size() == (size_t)_code.lambda_i)
//         {
//             StripeGroup stripe_group(sg_id, _code, _settings, stripes_in_group);
//             _stripe_groups.push_back(stripe_group);
//             sg_id++;
//             stripes_in_group.clear();
//         }
//     }

//     return true;
// }

// bool StripeBatch::constructBySendLoadAndCost(vector<Stripe> &stripes, mt19937 &random_generator)
// {

//     size_t num_stripes = stripes.size();
//     size_t num_nodes = _settings.M;

//     // check if the number of stripes is a multiple of lambda_i
//     if (num_stripes % _code.lambda_i != 0)
//     {
//         printf("invalid parameters\n");
//         return false;
//     }

//     // enumerate all possible stripe groups
//     vector<size_t> comb_ids;
//     vector<vector<size_t>> combinations = Utils::getCombinations(num_stripes, _code.lambda_i);
//     size_t num_candidate_sgs = combinations.size();

//     // record the candidate send load tables (and corresponding cost for each table) for each stripe group
//     vector<vector<vector<size_t>>> cand_sgs_slts(num_candidate_sgs, vector<vector<size_t>>());
//     vector<vector<int>> cand_sgs_costs(num_candidate_sgs, vector<int>()); // mark down the minimum cost for each candidate stripe group

//     vector<vector<size_t>> overlapped_cand_sgs(num_stripes, vector<size_t>()); // <stripe_id, overlapped_stripe_groups>

//     // enumerate send load tables and corresponding costs for all candidate stripe groups
//     for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++)
//     {
//         comb_ids.push_back(cand_id);

//         // construct candidate stripe group
//         vector<size_t> &comb = combinations[cand_id];

//         // record overlapped candidate stripe_groups
//         for (auto stripe_id : comb)
//         {
//             overlapped_cand_sgs[stripe_id].push_back(cand_id);
//         }

//         // construct stripe group
//         vector<Stripe *> stripes_in_group;
//         for (size_t sid = 0; sid < _code.lambda_i; sid++)
//         {
//             stripes_in_group.push_back(&stripes[comb[sid]]);
//         }
//         StripeGroup candidate_sg(cand_id, _code, _settings, stripes_in_group);

//         // get all possible transition send load table and their corresponding costs
//         cand_sgs_slts[cand_id] = candidate_sg.getCandSendLoadTables();
//         cand_sgs_costs[cand_id] = vector<int>(cand_sgs_slts[cand_id].size(), -1);
//         for (size_t slt_idx = 0; slt_idx < cand_sgs_slts[cand_id].size(); slt_idx++)
//         {
//             vector<size_t> &slt = cand_sgs_slts[cand_id][slt_idx];
//             int slt_cost = 0;
//             for (size_t node_id = 0; node_id < num_nodes; node_id++)
//             {
//                 slt_cost += slt[node_id];
//             }
//             cand_sgs_costs[cand_id][slt_idx] = slt_cost;
//         }
//     }

//     printf("total number of candidate stripe groups: %ld\n", num_candidate_sgs);

//     // for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++)
//     // {
//     //     printf("candidate_id: %ld\n", cand_id);
//     //     Utils::printUIntVector(combinations[cand_id]);
//     //     printf("candidate send load tables: %ld\n", cand_sgs_slts.size());
//     //     for (size_t cand_slt_id = 0; cand_slt_id < cand_sgs_slts[cand_id].size(); cand_slt_id++)
//     //     {
//     //         printf("slt: %ld, cost: %d\n", cand_slt_id, cand_sgs_costs[cand_id][cand_slt_id]);
//     //         Utils::printUIntVector(cand_sgs_slts[cand_id][cand_slt_id]);
//     //     }
//     // }

//     // store stripe groups as sorted stripe ids
//     vector<size_t> sorted_stripe_ids;

//     // maintain a send load table for selected stripe groups
//     vector<size_t> cur_slt(num_nodes, 0);

//     // record currently valid candidate stripe groups; init as all valid
//     vector<bool> is_cand_valid(num_candidate_sgs, true);
//     vector<size_t> cur_valid_cand_ids = comb_ids;

//     size_t num_sgs = num_stripes / _code.lambda_i;
//     for (size_t sg_id = 0; sg_id < num_sgs; sg_id++)
//     {
//         // get remaining valid candidates
//         vector<size_t> valid_cand_ids;

//         // filter out invalid candidates
//         for (auto cand_id : cur_valid_cand_ids)
//         {
//             if (is_cand_valid[cand_id] == true)
//             {
//                 valid_cand_ids.push_back(cand_id);
//             }
//         }

//         // for all stripe groups, find the minimum of maximum send load by adding each of the candidate send load tables

//         size_t best_mml = SIZE_MAX;
//         int best_mml_cost = INT_MAX;
//         vector<size_t> best_mml_cand_ids;
//         vector<vector<size_t>> best_mml_slts;
//         for (size_t idx = 0; idx < valid_cand_ids.size(); idx++)
//         {
//             size_t cand_id = valid_cand_ids[idx];
//             vector<vector<size_t>> &cand_slts = cand_sgs_slts[cand_id]; // send load tables
//             vector<int> cand_slt_costs = cand_sgs_costs[cand_id];       // corresponding costs

//             for (size_t slt_idx = 0; slt_idx < cand_slts.size(); slt_idx++)
//             {
//                 vector<size_t> slt_after_conn = cur_slt;

//                 // get candidate send load table for the stripe group
//                 vector<size_t> &cand_slt = cand_slts[slt_idx];
//                 int cand_slt_cost = cand_slt_costs[slt_idx];
//                 // compute send load table after connection
//                 for (size_t node_id = 0; node_id < num_nodes; node_id++)
//                 {
//                     slt_after_conn[node_id] += cand_slt[node_id];
//                 }

//                 size_t slt_mml_after_conn = *max_element(slt_after_conn.begin(), slt_after_conn.end()); // get minimum of maximum load

//                 if (slt_mml_after_conn < best_mml || (slt_mml_after_conn == best_mml && cand_slt_cost < best_mml_cost))
//                 {
//                     // update mml and cost
//                     best_mml = slt_mml_after_conn;
//                     best_mml_cost = cand_slt_cost;

//                     // re-init the candidate lists
//                     best_mml_cand_ids.clear();
//                     best_mml_cand_ids.push_back(cand_id);
//                     best_mml_slts.clear();
//                     best_mml_slts.push_back(cand_slt);
//                 }
//                 else if (slt_mml_after_conn == best_mml && cand_slt_cost == best_mml_cost)
//                 {
//                     // add as mml_min_cost candidates
//                     best_mml_cand_ids.push_back(cand_id);
//                     best_mml_slts.push_back(cand_slt);
//                 }
//             }
//         }

//         // randomly find a mml_min_cost candidate and its send load table
//         size_t min_max_load = best_mml;
//         int mml_min_cost = best_mml_cost;
//         size_t rand_pos = Utils::randomUInt(0, best_mml_cand_ids.size() - 1, random_generator);
//         size_t mml_min_cost_cand_id = best_mml_cand_ids[rand_pos];
//         vector<size_t> &mml_min_cost_slt = best_mml_slts[rand_pos];
//         vector<size_t> &mml_min_cost_comb = combinations[mml_min_cost_cand_id];

//         // update current send load table
//         for (size_t node_id = 0; node_id < num_nodes; node_id++)
//         {
//             cur_slt[node_id] += mml_min_cost_slt[node_id];
//         }

//         // update stripes info
//         for (auto stripe_id : mml_min_cost_comb)
//         {
//             // add stripes from the selected stripe group to sorted stripes
//             sorted_stripe_ids.push_back(stripe_id);

//             // mark overlapped candidate stripe groups as invalid
//             for (auto cand_sg_ids : overlapped_cand_sgs[stripe_id])
//             {
//                 is_cand_valid[cand_sg_ids] = false;
//             }
//         }

//         // update currently valid combinations
//         cur_valid_cand_ids = valid_cand_ids;

//         // summarize current pick
//         printf("pick candidate_id: %ld, min_max_load: %ld, mml_min_cost: %d, remaining number of candidates: (%ld / %ld), cur_slt:\n", mml_min_cost_cand_id, min_max_load, mml_min_cost, cur_valid_cand_ids.size(), num_candidate_sgs);
//         Utils::printUIntVector(cur_slt);
//     }

//     // if (sorted_stripe_ids.size() != num_stripes) {
//     //     printf("error: invalid sorted_stripe_ids\n");
//     //     return false;
//     // }

//     // 2. put stripes by sorted stripe ids (by cost) into stripe groups
//     vector<Stripe *> stripes_in_group;
//     size_t sg_id = 0; // stripe group id
//     for (size_t idx = 0; idx < num_stripes; idx++)
//     {

//         size_t stripe_id = sorted_stripe_ids[idx];

//         // add the stripe into stripe group
//         Stripe &stripe = stripes[stripe_id];
//         stripes_in_group.push_back(&stripe);

//         if (stripes_in_group.size() == (size_t)_code.lambda_i)
//         {
//             StripeGroup stripe_group(sg_id, _code, _settings, stripes_in_group);
//             _stripe_groups.push_back(stripe_group);
//             sg_id++;
//             stripes_in_group.clear();
//         }
//     }

//     return true;
// }

// bool StripeBatch::constructBySendLoadAndCostv2(vector<Stripe> &stripes, mt19937 &random_generator)
// {
//     size_t num_nodes = _settings.M;
//     size_t num_stripes = stripes.size();

//     _stripe_groups.clear();
//     _sg_approaches.clear();

//     // check if the number of stripes is a multiple of lambda_i
//     if (num_stripes % _code.lambda_i != 0)
//     {
//         printf("invalid parameters\n");
//         return false;
//     }

//     // enumerate all possible stripe groups
//     vector<size_t> cand_ids;
//     vector<vector<size_t>> combinations = Utils::getCombinations(num_stripes, _code.lambda_i);
//     size_t num_cand_sgs = combinations.size();

//     unordered_map<size_t, vector<LoadTable>> cand_sgs_slts; // all the candidate send load tables and their costs for each stripe group

//     // for each stripe, get all the candidate stripe groups that contains the stripe <stripe_id, overlapped_stripe_groups>
//     vector<vector<size_t>> overlapped_cand_sgs(num_stripes, vector<size_t>());

//     // enumerate send load tables and corresponding costs for all candidate stripe groups
//     for (size_t cand_id = 0; cand_id < num_cand_sgs; cand_id++)
//     {
//         // get stripe ids in the stripe group
//         vector<size_t> &comb = combinations[cand_id];
//         cand_ids.push_back(cand_id);

//         // record overlapped candidate stripe_groups
//         for (auto stripe_id : comb)
//         {
//             overlapped_cand_sgs[stripe_id].push_back(cand_id);
//         }

//         // construct stripe group
//         vector<Stripe *> stripes_in_group;
//         for (size_t sid = 0; sid < _code.lambda_i; sid++)
//         {
//             stripes_in_group.push_back(&stripes[comb[sid]]);
//         }
//         StripeGroup candidate_sg(cand_id, _code, _settings, stripes_in_group);

//         // get all possible transition send load table and their corresponding costs
//         cand_sgs_slts[cand_id] = candidate_sg.getCandSLTs();
//     }

//     printf("total number of candidate stripe groups: %ld\n", num_cand_sgs);

//     // maintain a send load table for currently selected stripe groups
//     LoadTable cur_slt;
//     cur_slt.lt = vector<size_t>(num_nodes, 0);
//     cur_slt.cost = 0;

//     // record currently valid candidate stripe groups; init as all valid
//     vector<bool> is_cand_valid(num_cand_sgs, true);
//     vector<size_t> cur_valid_cand_ids = cand_ids;

//     size_t num_sgs = num_stripes / _code.lambda_i;
//     for (size_t sg_id = 0; sg_id < num_sgs; sg_id++)
//     {
//         // get remaining valid candidates
//         vector<size_t> valid_cand_ids;

//         // filter out invalid candidates
//         for (auto cand_id : cur_valid_cand_ids)
//         {
//             if (is_cand_valid[cand_id] == true)
//             {
//                 valid_cand_ids.push_back(cand_id);
//             }
//         }

//         // for all stripe groups, find the minimum of maximum send load by adding each of the candidate send load tables

//         double best_metric = DBL_MAX; // first priority: load balance metric (maximum load)
//         // size_t best_metric = SIZE_MAX;   // first priority: load balance metric (maximum load)
//         size_t best_max_load = SIZE_MAX; // maximum load
//         size_t best_cost = SIZE_MAX;     // second priority: cost
//         vector<pair<size_t, LoadTable>> best_cands;
//         for (size_t idx = 0; idx < valid_cand_ids.size(); idx++)
//         {
//             size_t cand_id = valid_cand_ids[idx];
//             vector<LoadTable> &cand_slts = cand_sgs_slts[cand_id]; // send load tables

//             for (LoadTable &cand_slt : cand_slts)
//             {
//                 LoadTable slt_after = cur_slt;

//                 // get candidate send load table for the stripe group
//                 // compute send load table after connection
//                 slt_after.lt = Utils::dotAddUIntVectors(slt_after.lt, cand_slt.lt);
//                 slt_after.cost += cand_slt.cost;

//                 // get load metric

//                 // maximum load after applying the stripe group
//                 size_t slt_max_load_after = *max_element(slt_after.lt.begin(), slt_after.lt.end());

//                 // choice 2: load metric
//                 double slt_mean_load_after = 1.0 * accumulate(slt_after.lt.begin(), slt_after.lt.end(), 0) / num_nodes;

//                 // choice 1: maximum load
//                 double metric = slt_max_load_after;

//                 // // choice 2: load-balance metric ((max - avg) / avg)
//                 // double metric = 0;
//                 // if (slt_mean_load_after > 0)
//                 // {
//                 //     metric = slt_max_load_after / slt_mean_load_after;
//                 // }

//                 if (metric < best_metric || (metric == best_metric && cand_slt.cost < best_cost))
//                 {
//                     // update mml and cost
//                     best_metric = metric;
//                     best_max_load = slt_max_load_after;
//                     best_cost = cand_slt.cost;

//                     // re-init the candidate lists
//                     best_cands.clear();
//                     best_cands.push_back(pair<size_t, LoadTable>(cand_id, cand_slt));
//                 }
//                 else if (metric == best_metric && cand_slt.cost == best_cost)
//                 {
//                     // add as best candidates
//                     best_cands.push_back(pair<size_t, LoadTable>(cand_id, cand_slt));
//                 }
//             }
//         }

//         // randomly find a mml_min_cost candidate and its send load table
//         size_t rand_pos = Utils::randomUInt(0, best_cands.size() - 1, random_generator);
//         size_t best_cand_id = best_cands[rand_pos].first;
//         LoadTable &best_slt = best_cands[rand_pos].second;
//         vector<size_t> &best_comb = combinations[best_cand_id];

//         // add the selected best stripe group (with approach)
//         vector<Stripe *> stripes_in_group;
//         for (size_t sid = 0; sid < _code.lambda_i; sid++)
//         {
//             stripes_in_group.push_back(&stripes[best_comb[sid]]);
//         }
//         StripeGroup best_sg(_stripe_groups.size(), _code, _settings, stripes_in_group);
//         _stripe_groups.push_back(best_sg);
//         _sg_approaches.push_back(best_slt.approach);

//         // update current send load table
//         cur_slt.lt = Utils::dotAddUIntVectors(cur_slt.lt, best_slt.lt);
//         cur_slt.cost += best_slt.cost;

//         // update currently valid stripes
//         for (auto stripe_id : best_comb)
//         {
//             // mark overlapped candidate stripe groups as invalid
//             for (auto cand_sg_ids : overlapped_cand_sgs[stripe_id])
//             {
//                 is_cand_valid[cand_sg_ids] = false;
//             }
//         }

//         // update currently valid combinations
//         cur_valid_cand_ids = valid_cand_ids;

//         // summarize current pick
//         printf("pick candidate_id: %ld, approach: %d, best_load (max_load): %ld, best_cost: %ld, remaining number of candidates: (%ld / %ld), cur_slt:\n", best_cand_id, best_slt.approach, best_max_load, best_cost, cur_valid_cand_ids.size(), num_cand_sgs);
//         Utils::printUIntVector(cur_slt.lt);
//     }

//     return true;
// }
