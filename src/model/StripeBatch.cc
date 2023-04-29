#include "StripeBatch.hh"

StripeBatch::StripeBatch(size_t _id, ConvertibleCode &_code, ClusterSettings &_settings, mt19937 &_random_generator, vector<Stripe> &_stripes) : id(_id), code(_code), settings(_settings), random_generator(_random_generator), stripes(_stripes)
{
}

StripeBatch::~StripeBatch()
{
}

void StripeBatch::print()
{
    printf("StripeBatch %ld:\n", id);
    for (auto &stripe_group : stripe_groups)
    {
        stripe_group.print();
    }
}

bool StripeBatch::constructSGInSequence()
{
    size_t num_stripes = settings.N;
    size_t num_sgs = num_stripes / code.lambda_i;

    enumerated_stripe_groups = (StripeGroup *)malloc(num_sgs * sizeof(StripeGroup)); // only enumerate num_sgs stripe groups

    for (size_t sg_id = 0, stripe_id = 0; sg_id < num_sgs; sg_id++)
    {
        vector<Stripe *> stripes_in_group(code.lambda_i, NULL);
        for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
        {
            stripes_in_group[stripe_id] = &stripes[sg_id * code.lambda_i + stripe_id];
        }

        // TODO: make stripe group with smaller memory!!!!
        enumerated_stripe_groups[sg_id] = StripeGroup()
    }

    // put stripes sequentially into stripe groups
    for (size_t stripe_id = 0, sg_id = 0; stripe_id < stripes.size(); stripe_id++)
    {

        // add the stripe into stripe group
        Stripe &stripe = stripes[stripe_id];
        stripes_in_group.push_back(&stripe);

        if (stripes_in_group.size() == code.lambda_i)
        {
            StripeGroup stripe_group(sg_id, code, _settings, stripes_in_group);
            _stripe_groups.push_back(stripe_group);
            sg_id++;
            stripes_in_group.clear();
        }
    }

    return true;
}

bool StripeBatch::constructSGByRandomPick(mt19937 &random_generator)
{
    size_t num_stripes = stripes.size();

    // check if the number of stripes is a multiple of lambda_i
    if (num_stripes % _code.lambda_i != 0)
    {
        printf("invalid parameters\n");
        return false;
    }

    // mark if the stripes are selected
    vector<bool> stripes_selected(num_stripes, false);

    vector<Stripe *> stripes_in_group;
    size_t sg_id = 0; // stripe group id

    // put stripes randomly into stripe groups
    for (size_t idx = 0; idx < num_stripes; idx++)
    {

        // randomly pick a non-selected stripe into group
        size_t stripe_id = INVALID_ID;
        while (true)
        {
            stripe_id = Utils::randomUInt(0, num_stripes - 1, random_generator);
            if (stripes_selected[stripe_id] == false)
            {
                stripes_selected[stripe_id] = true;
                break;
            }
        }

        // add the stripe into stripe group
        Stripe &stripe = stripes[stripe_id];
        stripes_in_group.push_back(&stripe);

        if (stripes_in_group.size() == _code.lambda_i)
        {
            StripeGroup stripe_group(sg_id, _code, _settings, stripes_in_group);
            _stripe_groups.push_back(stripe_group);
            sg_id++;
            stripes_in_group.clear();
        }
    }

    return true;
}

bool StripeBatch::constructSGByCost(vector<Stripe> &stripes)
{
    size_t num_stripes = stripes.size();

    // check if the number of stripes is a multiple of lambda_i
    if (num_stripes % _code.lambda_i != 0)
    {
        printf("invalid parameters\n");
        return false;
    }

    // enumerate all possible stripe groups
    vector<size_t> comb_ids;
    vector<vector<size_t>> combinations = Utils::getCombinations(num_stripes, _code.lambda_i);
    size_t num_candidate_sgs = combinations.size();

    vector<int> candidate_sgs_costs(num_candidate_sgs, -1); // mark down the minimum cost for each candidate stripe group

    vector<vector<size_t>> overlapped_cand_sgs(num_stripes, vector<size_t>()); // <stripe_id, overlapped_stripe_groups>

    for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++)
    {
        comb_ids.push_back(cand_id);

        // construct candidate stripe group
        vector<size_t> &comb = combinations[cand_id];

        // record overlapped candidate stripe_groups
        for (auto stripe_id : comb)
        {
            overlapped_cand_sgs[stripe_id].push_back(cand_id);
        }

        vector<Stripe *> stripes_in_group;
        for (size_t sid = 0; sid < _code.lambda_i; sid++)
        {
            stripes_in_group.push_back(&stripes[comb[sid]]);
        }

        StripeGroup candidate_sg(cand_id, _code, _settings, stripes_in_group);

        // get transition cost
        int transition_cost = candidate_sg.getMinTransitionCost();
        candidate_sgs_costs[cand_id] = transition_cost;
    }

    printf("total number of candidate stripe groups: %ld\n", num_candidate_sgs);
    // for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++)
    // {
    //     // printf("candidate_id: %ld, cost: %d\n", cand_id, candidate_sgs_costs[cand_id]);
    //     // Utils::printUIntVector(combinations[cand_id]);
    //     printf("stripe (%ld, %ld) merge cost: %d\n", combinations[cand_id][0], combinations[cand_id][1], candidate_sgs_costs[cand_id]);
    // }

    vector<size_t> sorted_stripe_ids; // store sorted stripe ids

    // record currently valid candidates; initialize as all candidate sgs
    vector<bool> is_cand_valid(num_candidate_sgs, true);
    vector<size_t> cur_valid_cand_ids = comb_ids;
    vector<int> cur_valid_cand_costs = candidate_sgs_costs;

    size_t num_sgs = num_stripes / _code.lambda_i;
    for (size_t sg_id = 0; sg_id < num_sgs; sg_id++)
    {
        // get remaining valid combinations
        vector<size_t> valid_cand_ids;
        vector<int> valid_cand_costs;

        // filter out invalid combinations
        for (size_t idx = 0; idx < cur_valid_cand_ids.size(); idx++)
        {
            size_t cand_id = cur_valid_cand_ids[idx];
            if (is_cand_valid[cand_id] == true)
            {
                valid_cand_ids.push_back(cand_id);
                valid_cand_costs.push_back(candidate_sgs_costs[cand_id]);
            }
        }

        // get stripe group with minimum transition cost
        size_t min_cost_id = distance(valid_cand_costs.begin(), min_element(valid_cand_costs.begin(), valid_cand_costs.end()));
        size_t min_cost_cand_id = valid_cand_ids[min_cost_id];
        vector<size_t> &min_cost_comb = combinations[min_cost_cand_id];

        for (auto stripe_id : min_cost_comb)
        {
            // add stripes from the selected stripe group to sorted stripes
            sorted_stripe_ids.push_back(stripe_id);

            // mark overlapped candidate stripe groups as invalid
            for (auto cand_sg_ids : overlapped_cand_sgs[stripe_id])
            {
                is_cand_valid[cand_sg_ids] = false;
            }
        }

        // update currently valid combinations
        cur_valid_cand_ids = valid_cand_ids;
        cur_valid_cand_costs = valid_cand_costs;

        // summarize current pick
        printf("pick candidate_id: %ld, cost: %d, remaining number of candidates: (%ld / %ld)\n", min_cost_cand_id, valid_cand_costs[min_cost_id], cur_valid_cand_ids.size(), num_candidate_sgs);
    }

    // if (sorted_stripe_ids.size() != num_stripes) {
    //     printf("error: invalid sorted_stripe_ids\n");
    //     return false;
    // }

    // 2. put stripes by sorted stripe ids (by cost) into stripe groups
    vector<Stripe *> stripes_in_group;
    size_t sg_id = 0; // stripe group id
    for (size_t idx = 0; idx < num_stripes; idx++)
    {

        size_t stripe_id = sorted_stripe_ids[idx];

        // add the stripe into stripe group
        Stripe &stripe = stripes[stripe_id];
        stripes_in_group.push_back(&stripe);

        if (stripes_in_group.size() == (size_t)_code.lambda_i)
        {
            StripeGroup stripe_group(sg_id, _code, _settings, stripes_in_group);
            _stripe_groups.push_back(stripe_group);
            sg_id++;
            stripes_in_group.clear();
        }
    }

    return true;
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