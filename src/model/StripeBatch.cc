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

ConvertibleCode &StripeBatch::getCode() {
    return _code;
}

ClusterSettings &StripeBatch::getClusterSettings() {
    return _settings;
}

vector<StripeGroup> &StripeBatch::getStripeGroups() {
    return _stripe_groups;
}

size_t StripeBatch::getId() {
    return _id;
}

void StripeBatch::print() {
    printf("StripeBatch %ld:\n", _id);
    for (auto &stripe_group : _stripe_groups) {
        stripe_group.print();
    }
}

bool StripeBatch::constructInSequence(vector<Stripe> &stripes) {
    // check if the number of stripes is a multiple of lambda_i
    if (stripes.size() % _code.lambda_i != 0) {
        printf("invalid parameters\n");
        return false;
    }

    vector<Stripe *> stripes_in_group;
    size_t sg_id = 0;

    // put stripes sequentially into stripe groups
    for (size_t stripe_id = 0; stripe_id < stripes.size(); stripe_id++) {

        // add the stripe into stripe group
        Stripe &stripe = stripes[stripe_id];
        stripes_in_group.push_back(&stripe);

        if (stripes_in_group.size() == _code.lambda_i) {
            StripeGroup stripe_group(sg_id, _code, _settings, stripes_in_group);
            _stripe_groups.push_back(stripe_group);
            sg_id++;
            stripes_in_group.clear();
        }
    }

    return true;
}

bool StripeBatch::constructByRandomPick(vector<Stripe> &stripes, mt19937 &random_generator) {
    size_t num_stripes = stripes.size();

    // check if the number of stripes is a multiple of lambda_i
    if (num_stripes % _code.lambda_i != 0) {
        printf("invalid parameters\n");
        return false;
    }

    // mark if the stripes are selected
    vector<bool> stripes_selected(num_stripes, false);

    vector<Stripe *> stripes_in_group;
    size_t sg_id = 0; // stripe group id

    // put stripes randomly into stripe groups
    for (size_t idx = 0; idx < num_stripes; idx++) {

        // randomly pick a non-selected stripe into group
        size_t stripe_id = INVALID_ID;
        while (true) {
            stripe_id = Utils::randomUInt(0, num_stripes - 1, random_generator);
            if (stripes_selected[stripe_id] == false) {
                stripes_selected[stripe_id] = true;
                break;
            }
        }

        // add the stripe into stripe group
        Stripe &stripe = stripes[stripe_id];
        stripes_in_group.push_back(&stripe);

        if (stripes_in_group.size() == _code.lambda_i) {
            StripeGroup stripe_group(sg_id, _code, _settings, stripes_in_group);
            _stripe_groups.push_back(stripe_group);
            sg_id++;
            stripes_in_group.clear();
        }
    }

    return true;
}

bool StripeBatch::constructByCost(vector<Stripe> &stripes) {
    size_t num_stripes = stripes.size();

    // check if the number of stripes is a multiple of lambda_i
    if (num_stripes % _code.lambda_i != 0) {
        printf("invalid parameters\n");
        return false;
    }

    // enumerate all possible stripe groups
    vector<size_t> comb_ids;
    vector<vector<size_t> > combinations = Utils::getCombinations(num_stripes, _code.lambda_i);
    size_t num_candidate_sgs = combinations.size();
    
    vector<int> candidate_sgs_costs(num_candidate_sgs, -1); // mark down the minimum cost for each candidate stripe group

    vector<vector<size_t> > overlapped_cand_sgs(num_stripes, vector<size_t>()); // <stripe_id, overlapped_stripe_groups>

    for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++) {
        comb_ids.push_back(cand_id);

        // construct candidate stripe group
        vector<size_t> &comb = combinations[cand_id];

        // record overlapped candidate stripe_groups
        for (auto stripe_id : comb) {
            overlapped_cand_sgs[stripe_id].push_back(cand_id);
        }

        vector<Stripe *> stripes_in_group;
        for (size_t sid = 0; sid < _code.lambda_i; sid++) {
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
    for (size_t sg_id = 0; sg_id < num_sgs; sg_id++) {
        // get remaining valid combinations
        vector<size_t> valid_cand_ids;
        vector<int> valid_cand_costs;

        // filter out invalid combinations
        for (size_t idx = 0; idx < cur_valid_cand_ids.size(); idx++) {
            size_t cand_id = cur_valid_cand_ids[idx];
            if (is_cand_valid[cand_id] == true) {
                valid_cand_ids.push_back(cand_id);
                valid_cand_costs.push_back(candidate_sgs_costs[cand_id]);
            }
        }

        // get stripe group with minimum transition cost
        size_t min_cost_id = distance(valid_cand_costs.begin(), min_element(valid_cand_costs.begin(), valid_cand_costs.end()));
        size_t min_cost_cand_id = valid_cand_ids[min_cost_id];
        vector<size_t> &min_cost_comb = combinations[min_cost_cand_id];

        for (auto stripe_id : min_cost_comb) {
            // add stripes from the selected stripe group to sorted stripes
            sorted_stripe_ids.push_back(stripe_id);

            // mark overlapped candidate stripe groups as invalid
            for (auto cand_sg_ids : overlapped_cand_sgs[stripe_id]) {
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
    for (size_t idx = 0; idx < num_stripes; idx++) {

        size_t stripe_id = sorted_stripe_ids[idx];

        // add the stripe into stripe group
        Stripe &stripe = stripes[stripe_id];
        stripes_in_group.push_back(&stripe);

        if (stripes_in_group.size() == (size_t) _code.lambda_i) {
            StripeGroup stripe_group(sg_id, _code, _settings, stripes_in_group);
            _stripe_groups.push_back(stripe_group);
            sg_id++;
            stripes_in_group.clear();
        }
    }

    return true;
}

bool StripeBatch::constructByCostAndSendLoad(vector<Stripe> &stripes) {
    // maintain a send load table

    size_t num_stripes = stripes.size();

    // check if the number of stripes is a multiple of lambda_i
    if (num_stripes % _code.lambda_i != 0) {
        printf("invalid parameters\n");
        return false;
    }

    // send load table
    size_t num_nodes = _settings.M;
    vector<size_t> send_load_table(num_nodes, 0);

    // enumerate all possible stripe groups
    vector<size_t> comb_ids;
    vector<vector<size_t> > combinations = Utils::getCombinations(num_stripes, _code.lambda_i);
    size_t num_candidate_sgs = combinations.size();
    
    vector<int> candidate_sgs_costs(num_candidate_sgs, -1); // mark down the minimum cost for each candidate stripe group

    vector<vector<size_t> > overlapped_cand_sgs(num_stripes, vector<size_t>()); // <stripe_id, overlapped_stripe_groups>

    for (size_t cand_id = 0; cand_id < num_candidate_sgs; cand_id++) {
        comb_ids.push_back(cand_id);

        // construct candidate stripe group
        vector<size_t> &comb = combinations[cand_id];

        // record overlapped candidate stripe_groups
        for (auto stripe_id : comb) {
            overlapped_cand_sgs[stripe_id].push_back(cand_id);
        }

        vector<Stripe *> stripes_in_group;
        for (size_t sid = 0; sid < _code.lambda_i; sid++) {
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
    for (size_t sg_id = 0; sg_id < num_sgs; sg_id++) {
        // get remaining valid combinations
        vector<size_t> valid_cand_ids;
        vector<int> valid_cand_costs;

        // filter out invalid combinations
        for (size_t idx = 0; idx < cur_valid_cand_ids.size(); idx++) {
            size_t cand_id = cur_valid_cand_ids[idx];
            if (is_cand_valid[cand_id] == true) {
                valid_cand_ids.push_back(cand_id);
                valid_cand_costs.push_back(candidate_sgs_costs[cand_id]);
            }
        }

        // TODO: find the minimum cost
        // filter out all stripe groups with minimum transition cost
        // find the one with minimum load
        // update table

        // get stripe group with minimum transition cost
        size_t min_cost_id = distance(valid_cand_costs.begin(), min_element(valid_cand_costs.begin(), valid_cand_costs.end()));
        size_t min_cost_cand_id = valid_cand_ids[min_cost_id];
        vector<size_t> &min_cost_comb = combinations[min_cost_cand_id];

        for (auto stripe_id : min_cost_comb) {
            // add stripes from the selected stripe group to sorted stripes
            sorted_stripe_ids.push_back(stripe_id);

            // mark overlapped candidate stripe groups as invalid
            for (auto cand_sg_ids : overlapped_cand_sgs[stripe_id]) {
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
    for (size_t idx = 0; idx < num_stripes; idx++) {

        size_t stripe_id = sorted_stripe_ids[idx];

        // add the stripe into stripe group
        Stripe &stripe = stripes[stripe_id];
        stripes_in_group.push_back(&stripe);

        if (stripes_in_group.size() == (size_t) _code.lambda_i) {
            StripeGroup stripe_group(sg_id, _code, _settings, stripes_in_group);
            _stripe_groups.push_back(stripe_group);
            sg_id++;
            stripes_in_group.clear();
        }
    }

    return true;
}
