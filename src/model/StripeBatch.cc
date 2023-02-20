#include "StripeBatch.hh"

StripeBatch::StripeBatch(int id, ConvertibleCode &code, ClusterSettings &settings)
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

int StripeBatch::getId() {
    return _id;
}

void StripeBatch::print() {
    printf("StripeBatch %d:\n", _id);
    for (auto &stripe_group : _stripe_groups) {
        stripe_group.print();
    }
}

bool StripeBatch::constructInSequence(vector<Stripe> &stripes) {

    if (stripes.size() <= 0) {
        printf("invalid parameters\n");
        return false;
    }

    // check if the number of stripes is a multiple of lambda_i
    if (stripes.size() % _code.lambda_i != 0) {
        printf("invalid parameters\n");
        return false;
    }

    vector<Stripe *> stripes_in_group;
    int sg_id = 0;

    // put stripes sequentially into stripe groups
    for (size_t stripe_id = 0; stripe_id < stripes.size(); stripe_id++) {

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

bool StripeBatch::constructByRandomPick(vector<Stripe> &stripes, mt19937 &random_generator) {

    if (stripes.size() <= 0) {
        printf("invalid parameters\n");
        return false;
    }

    int num_stripes = stripes.size();

    // check if the number of stripes is a multiple of lambda_i
    if (num_stripes % _code.lambda_i != 0) {
        printf("invalid parameters\n");
        return false;
    }

    // mark if the stripes are selected
    vector<bool> stripes_selected(num_stripes, false);

    vector<Stripe *> stripes_in_group;
    int sg_id = 0; // stripe group id

    // put stripes randomly into stripe groups
    for (int idx = 0; idx < num_stripes; idx++) {

        // randomly pick a non-selected stripe into group
        int stripe_id = -1;
        while (true) {
            stripe_id = Utils::randomInt(0, num_stripes - 1, random_generator);
            if (stripes_selected[stripe_id] == false) {
                stripes_selected[stripe_id] = true;
                break;
            }
        }

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

bool StripeBatch::constructByCost(vector<Stripe> &stripes) {

    if (stripes.size() <= 0) {
        printf("invalid parameters\n");
        return false;
    }

    int num_stripes = stripes.size();

    // check if the number of stripes is a multiple of lambda_i
    if (num_stripes % _code.lambda_i != 0) {
        printf("invalid parameters\n");
        return false;
    }

    // enumerate all possible stripe groups
    vector<int> comb_ids;
    vector<vector<int> > combinations = Utils::getCombinations(num_stripes, _code.lambda_i);
    int num_candidate_sgs = combinations.size();
    
    vector<int> candidate_sgs_costs(num_candidate_sgs, -1); // mark down the minimum cost for each candidate stripe group

    vector<vector<int> > overlapped_cand_sgs(num_stripes, vector<int>()); // <stripe_id, overlapped_stripe_groups>

    for (int cand_id = 0; cand_id < num_candidate_sgs; cand_id++) {
        comb_ids.push_back(cand_id);

        // construct candidate stripe group
        vector<int> &comb = combinations[cand_id];

        // record overlapped candidate stripe_groups
        for (auto stripe_id : comb) {
            overlapped_cand_sgs[stripe_id].push_back(cand_id);
        }

        vector<Stripe *> stripes_in_group;
        for (int sid = 0; sid < _code.lambda_i; sid++) {
            stripes_in_group.push_back(&stripes[comb[sid]]);
        }

        StripeGroup candidate_sg(cand_id, _code, _settings, stripes_in_group);

        // get transition cost
        int transition_cost = candidate_sg.getMinTransitionCost();
        candidate_sgs_costs[cand_id] = transition_cost;
    }

    printf("total number of candidate stripe groups: %d\n", num_candidate_sgs);
    // for (int cand_id = 0; cand_id < num_candidate_sgs; cand_id++) {
    //     printf("candidate_id: %d, cost: %d\n", cand_id, candidate_sgs_costs[cand_id]);
    //     Utils::printIntVector(combinations[cand_id]);
    // }

    vector<int> sorted_stripe_ids; // store sorted stripe ids

    // record currently valid candidates; initialize as all candidate sgs
    vector<bool> is_cand_valid(num_candidate_sgs, true);
    vector<int> cur_valid_cand_ids = comb_ids;
    vector<int> cur_valid_cand_costs = candidate_sgs_costs;

    int num_sgs = num_stripes / _code.lambda_i;
    for (int sg_id = 0; sg_id < num_sgs; sg_id++) {
        // get remaining valid combinations
        vector<int> valid_cand_ids;
        vector<int> valid_cand_costs;

        // filter out invalid combinations
        for (size_t idx = 0; idx < cur_valid_cand_ids.size(); idx++) {
            int cand_id = cur_valid_cand_ids[idx];
            if (is_cand_valid[cand_id] == true) {
                valid_cand_ids.push_back(cand_id);
                valid_cand_costs.push_back(candidate_sgs_costs[cand_id]);
            }
        }

        // get stripe group with minimum transition cost
        int min_cost_id = distance(valid_cand_costs.begin(), min_element(valid_cand_costs.begin(), valid_cand_costs.end()));
        int min_cost_cand_id = valid_cand_ids[min_cost_id];
        vector<int> &min_cost_comb = combinations[min_cost_cand_id];

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
        printf("pick candidate_id: %d, cost: %d, remaining number of candidates: (%ld / %d)\n", min_cost_cand_id, valid_cand_costs[min_cost_id], cur_valid_cand_ids.size(), num_candidate_sgs);
    }
        
    // if (sorted_stripe_ids.size() != (size_t) num_stripes) {
    //     printf("error: invalid sorted_stripe_ids\n");
    //     return false;
    // }

    // 2. put stripes by sorted stripe ids (by cost) into stripe groups
    vector<Stripe *> stripes_in_group;
    int sg_id = 0; // stripe group id
    for (int idx = 0; idx < num_stripes; idx++) {

        int stripe_id = sorted_stripe_ids[idx];

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
