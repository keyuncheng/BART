#include "StripeBatch.hh"

StripeBatch::StripeBatch(size_t id, ConvertibleCode &code, ClusterSettings &settings)
{
    _id = id;
    _code = code;
    _settings = settings;
}

StripeBatch::~StripeBatch()
{
}

ConvertibleCode &StripeBatch::getCode()
{
    return _code;
}

ClusterSettings &StripeBatch::getClusterSettings()
{
    return _settings;
}

vector<StripeGroup> &StripeBatch::getStripeGroups()
{
    return _stripe_groups;
}

size_t StripeBatch::getId()
{
    return _id;
}

void StripeBatch::print()
{
    printf("StripeBatch %ld:\n", _id);
    for (auto &stripe_group : _stripe_groups)
    {
        stripe_group.print();
    }
}

bool StripeBatch::constructInSequence(vector<Stripe> &stripes)
{
    // check if the number of stripes is a multiple of lambda_i
    if (stripes.size() % _code.lambda_i != 0)
    {
        printf("invalid parameters\n");
        return false;
    }

    vector<Stripe *> stripes_in_group;
    size_t sg_id = 0;

    // put stripes sequentially into stripe groups
    for (size_t stripe_id = 0; stripe_id < stripes.size(); stripe_id++)
    {

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

bool StripeBatch::constructByRandomPick(vector<Stripe> &stripes, mt19937 &random_generator)
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

bool StripeBatch::constructByCost(vector<Stripe> &stripes)
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
    // for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++) {
    //     printf("candidate_id: %ld, cost: %d\n", cand_id, candidate_sgs_costs[cand_id]);
    //     Utils::printUIntVector(combinations[cand_id]);
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

bool StripeBatch::constructByCostAndSendLoad(vector<Stripe> &stripes)
{

    size_t num_stripes = stripes.size();

    // check if the number of stripes is a multiple of lambda_i
    if (num_stripes % _code.lambda_i != 0)
    {
        printf("invalid parameters\n");
        return false;
    }

    // maintain a send load table
    size_t num_nodes = _settings.M;
    vector<size_t> cur_send_load_table(num_nodes, 0);

    // enumerate all possible stripe groups
    vector<size_t> comb_ids;
    vector<vector<size_t>> combinations = Utils::getCombinations(num_stripes, _code.lambda_i);
    size_t num_candidate_sgs = combinations.size();

    vector<int> candidate_sgs_costs(num_candidate_sgs, -1); // mark down the minimum cost for each candidate stripe group

    vector<vector<size_t>> overlapped_cand_sgs(num_stripes, vector<size_t>()); // <stripe_id, overlapped_stripe_groups>

    // enumerate transition costs for all candidate stripe groups
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
    // for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++) {
    //     printf("candidate_id: %ld, cost: %d\n", cand_id, candidate_sgs_costs[cand_id]);
    //     Utils::printUIntVector(combinations[cand_id]);
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

        // filter out stripe groups with minimum transition cost
        int min_cost = *min_element(valid_cand_costs.begin(), valid_cand_costs.end());

        vector<size_t> min_cost_cand_ids;
        for (size_t idx = 0; idx < valid_cand_ids.size(); idx++)
        {
            if (valid_cand_costs[idx] == min_cost)
            {
                min_cost_cand_ids.push_back(valid_cand_ids[idx]);
            }
        }
        // find stripe groups with <minimum load after connection> among all min cost candidates
        vector<size_t> cand_min_cost_loads(min_cost_cand_ids.size(), 0);
        vector<vector<size_t>> cand_min_cost_slt(min_cost_cand_ids.size(), vector<size_t>());

        for (size_t idx = 0; idx < min_cost_cand_ids.size(); idx++)
        {
            size_t min_load_cand_id = min_cost_cand_ids[idx];
            // construct candidate stripe group
            vector<size_t> &comb = combinations[min_load_cand_id];

            vector<Stripe *> stripes_in_group;
            for (size_t sid = 0; sid < _code.lambda_i; sid++)
            {
                stripes_in_group.push_back(&stripes[comb[sid]]);
            }

            StripeGroup candidate_sg(min_load_cand_id, _code, _settings, stripes_in_group);

            vector<vector<size_t>> cand_send_load_tables = candidate_sg.getCandSendLoadTablesForMinTransCost(min_cost);

            size_t best_min_max_load = SIZE_MAX;
            vector<size_t> best_min_max_load_slt;
            for (auto &send_load_table : cand_send_load_tables)
            {
                vector<size_t> send_load_table_after_connection = cur_send_load_table;
                for (size_t node_id = 0; node_id < num_nodes; node_id++)
                {
                    send_load_table_after_connection[node_id] += send_load_table[node_id];
                }

                size_t cur_min_max_load = *max_element(send_load_table_after_connection.begin(), send_load_table_after_connection.end());

                if (cur_min_max_load < best_min_max_load)
                {
                    best_min_max_load = cur_min_max_load;
                    best_min_max_load_slt = send_load_table;
                }
            }

            cand_min_cost_loads[idx] = best_min_max_load;
            cand_min_cost_slt[idx] = best_min_max_load_slt;
        }

        // from candidate combinations, find the min_max load increase
        size_t min_cost_min_max_load_idx = distance(cand_min_cost_loads.begin(), min_element(cand_min_cost_loads.begin(), cand_min_cost_loads.end()));
        size_t min_cost_min_load = cand_min_cost_loads[min_cost_min_max_load_idx];
        size_t min_cost_min_max_load_cand_id = min_cost_cand_ids[min_cost_min_max_load_idx];
        vector<size_t> min_cost_min_max_load_slt = cand_min_cost_slt[min_cost_min_max_load_idx];
        vector<size_t> &min_cost_min_max_load_comb = combinations[min_cost_min_max_load_cand_id];

        // update current load table
        for (size_t node_id = 0; node_id < num_nodes; node_id++)
        {
            cur_send_load_table[node_id] += min_cost_min_max_load_slt[node_id];
        }

        for (auto stripe_id : min_cost_min_max_load_comb)
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
        printf("pick candidate_id: %ld, cost: %d, min_cost_min_load: %ld, remaining number of candidates: (%ld / %ld), cur_send_load_table:\n", min_cost_min_max_load_cand_id, min_cost, min_cost_min_load, cur_valid_cand_ids.size(), num_candidate_sgs);
        Utils::printUIntVector(cur_send_load_table);
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

bool StripeBatch::constructBySendLoadAndCost(vector<Stripe> &stripes)
{

    size_t num_stripes = stripes.size();
    size_t num_nodes = _settings.M;

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

    // record the candidate send load tables (and corresponding cost for each table) for each stripe group
    vector<vector<vector<size_t>>> cand_sgs_slt(num_candidate_sgs, vector<vector<size_t>>());
    vector<vector<int>> cand_sgs_costs(num_candidate_sgs, vector<int>()); // mark down the minimum cost for each candidate stripe group

    vector<vector<size_t>> overlapped_cand_sgs(num_stripes, vector<size_t>()); // <stripe_id, overlapped_stripe_groups>

    // enumerate send load tables and corresponding costs for all candidate stripe groups
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

        // construct stripe group
        vector<Stripe *> stripes_in_group;
        for (size_t sid = 0; sid < _code.lambda_i; sid++)
        {
            stripes_in_group.push_back(&stripes[comb[sid]]);
        }
        StripeGroup candidate_sg(cand_id, _code, _settings, stripes_in_group);

        // get all possible transition send load table and their corresponding costs
        cand_sgs_slt[cand_id] = candidate_sg.getCandSendLoadTables();
        cand_sgs_costs[cand_id] = vector<int>(cand_sgs_slt[cand_id].size(), -1);
        for (size_t slt_idx = 0; slt_idx < cand_sgs_slt[cand_id].size(); slt_idx++)
        {
            vector<size_t> &slt = cand_sgs_slt[cand_id][slt_idx];
            int slt_cost = 0;
            for (size_t node_id = 0; node_id < num_nodes; node_id++)
            {
                slt_cost += slt[node_id];
            }
            cand_sgs_costs[cand_id][slt_idx] = slt_cost;
        }
    }

    printf("total number of candidate stripe groups: %ld\n", num_candidate_sgs);

    // maintain a send load table
    vector<size_t> cur_send_load_table(num_nodes, 0);

    vector<size_t> sorted_stripe_ids; // store sorted stripe ids

    // record currently valid candidates; initialize as all candidate sgs
    vector<bool> is_cand_valid(num_candidate_sgs, true);
    vector<size_t> cur_valid_cand_ids = comb_ids;

    size_t num_sgs = num_stripes / _code.lambda_i;
    for (size_t sg_id = 0; sg_id < num_sgs; sg_id++)
    {
        // get remaining valid combinations
        vector<size_t> valid_cand_ids;

        // filter out invalid combinations
        for (size_t idx = 0; idx < cur_valid_cand_ids.size(); idx++)
        {
            size_t cand_id = cur_valid_cand_ids[idx];
            if (is_cand_valid[cand_id] == true)
            {
                valid_cand_ids.push_back(cand_id);
            }
        }

        // for each stripe group, record the maximum send load after connection
        vector<size_t> valid_cand_max_load_after_conn(valid_cand_ids.size(), 0);
        vector<vector<size_t>> valid_cand_max_load_slt(valid_cand_ids.size(), vector<size_t>());
        vector<int> valid_cand_max_load_costs(valid_cand_ids.size(), -1);

        for (size_t idx = 0; idx < valid_cand_ids.size(); idx++)
        {
            size_t cand_id = valid_cand_ids[idx];
            vector<vector<size_t>> &cand_slts = cand_sgs_slt[cand_id];
            vector<int> cand_slt_costs = cand_sgs_costs[cand_id];

            size_t best_min_max_load = SIZE_MAX;
            vector<size_t> best_min_max_load_slt;
            int best_mml_cost = -1;
            for (size_t slt_idx = 0; slt_idx < cand_slts.size(); slt_idx++)
            {
                vector<size_t> &slt = cand_slts[slt_idx];
                vector<size_t> cur_slt_after_conn = cur_send_load_table;
                for (size_t node_id = 0; node_id < num_nodes; node_id++)
                {
                    cur_slt_after_conn[node_id] += slt[node_id];
                }

                size_t cur_min_max_load = *max_element(cur_slt_after_conn.begin(), cur_slt_after_conn.end());

                if (cur_min_max_load < best_min_max_load)
                {
                    best_min_max_load = cur_min_max_load;
                    best_min_max_load_slt = slt;
                    best_mml_cost = cand_slt_costs[slt_idx];
                }
            }

            valid_cand_max_load_after_conn[idx] = best_min_max_load;
            valid_cand_max_load_slt[idx] = best_min_max_load_slt;
            valid_cand_max_load_costs[idx] = best_mml_cost;
        }

        // filter out stripe groups with minimum send load after connection
        size_t min_max_load = *min_element(valid_cand_max_load_after_conn.begin(), valid_cand_max_load_after_conn.end());

        vector<size_t> mml_cand_ids;
        vector<vector<size_t>> mml_slts;
        vector<int> mml_costs;
        for (size_t idx = 0; idx < valid_cand_ids.size(); idx++)
        {
            if (valid_cand_max_load_after_conn[idx] == min_max_load)
            {
                size_t cand_id = valid_cand_ids[idx];
                mml_cand_ids.push_back(cand_id);
                mml_slts.push_back(valid_cand_max_load_slt[idx]);
                mml_costs.push_back(valid_cand_max_load_costs[idx]);
            }
        }
        // find stripe groups with <minimum costs> among all min cost candidates
        size_t mml_min_cost_idx = distance(mml_costs.begin(), min_element(mml_costs.begin(), mml_costs.end()));
        int mml_min_cost = mml_costs[mml_min_cost_idx];
        size_t mml_min_cost_cand_id = mml_cand_ids[mml_min_cost_idx];
        vector<size_t> mml_min_cost_slt = mml_slts[mml_min_cost_idx];
        vector<size_t> &mml_min_cost_comb = combinations[mml_min_cost_cand_id];

        // update current load table
        for (size_t node_id = 0; node_id < num_nodes; node_id++)
        {
            cur_send_load_table[node_id] += mml_min_cost_slt[node_id];
        }

        for (auto stripe_id : mml_min_cost_comb)
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

        // summarize current pick
        printf("pick candidate_id: %ld, min_max_load: %ld, mml_min_cost: %d, remaining number of candidates: (%ld / %ld), cur_send_load_table:\n", mml_min_cost_cand_id, min_max_load, mml_min_cost, cur_valid_cand_ids.size(), num_candidate_sgs);
        Utils::printUIntVector(cur_send_load_table);
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