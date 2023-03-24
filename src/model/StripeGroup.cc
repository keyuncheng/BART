#include "StripeGroup.hh"

StripeGroup::StripeGroup(size_t id, ConvertibleCode &code, ClusterSettings &settings, vector<Stripe *> &stripes)
{
    _id = id;
    _code = code;
    _settings = settings;
    _stripes = stripes;
}

StripeGroup::~StripeGroup()
{
}

vector<size_t> StripeGroup::getDataDistribution() {
    size_t num_nodes = _settings.M;

    vector<size_t> data_distribution(num_nodes, 0);

    for (auto stripe : _stripes) {
        vector<size_t> &stripe_indices = stripe->getStripeIndices();

        for (size_t block_id = 0; block_id < _code.k_i; block_id++) {
            data_distribution[stripe_indices[block_id]] += 1;
        }
    }

    return data_distribution;
}

vector<vector<size_t> > StripeGroup::getParityDistributions() {
    vector<vector<size_t> > parity_distributions;

    for (size_t parity_id = 0; parity_id < _code.m_i; parity_id++) {
        parity_distributions.push_back(getParityDistribution(parity_id));
    }

    return parity_distributions;
}

vector<size_t> StripeGroup::getParityDistribution(size_t parity_id) {
    size_t num_nodes = _settings.M;
    vector<size_t> parity_distribution(num_nodes, 0);

    // check which node already stores at least one data block
    for (auto stripe : _stripes) {
        vector<size_t> &stripe_indices = stripe->getStripeIndices();
        size_t parity_node_id = stripe_indices[_code.k_i + parity_id];
        parity_distribution[parity_node_id] += 1;
    }

    return parity_distribution;
}

ConvertibleCode &StripeGroup::getCode() {
    return _code;
}

ClusterSettings &StripeGroup::getClusterSettings() {
    return _settings;
}

vector<Stripe *> &StripeGroup::getStripes() {
    return _stripes;
}

size_t StripeGroup::getId() {
    return _id;
}

void StripeGroup::print() {
    printf("Stripe group %ld:\n", _id);
    for (auto stripe : _stripes) {
        stripe->print();
    }
}

int StripeGroup::getMinTransitionCost() {

    // data relocation cost
    int data_reloc_cost = getDataRelocationCost();
    int parity_update_cost = 0;

    // approach 1: re-encoding cost (baseline)
    int re_cost = getMinReEncodingCost();
    parity_update_cost = re_cost;

    int pm_cost = -1;

    if (_code.isValidForPM() == true) {
        // approach 2: parity merging
        pm_cost = getMinParityMergingCost();
        if (pm_cost < re_cost) {
            parity_update_cost = pm_cost;
        }
    }

    // printf("data_relocation_cost: %d, parity_update_cost: %d, re-encoding cost: %d, parity merging cost: %d\n", data_reloc_cost, parity_update_cost, re_cost, pm_cost);

    return data_reloc_cost + parity_update_cost;
}

int StripeGroup::getDataRelocationCost() {
    size_t num_nodes = _settings.M;

    int data_relocation_cost = 0;
    vector<size_t> data_distribution = getDataDistribution();

    // printf("data_distribution:\n");
    // Utils::printIntVector(data_distribution);

    // it targets at general parameters (meaning that for a converted group, there must exist no more than lambda_f data blocks in a node)
    for (size_t node_id = 0; node_id < num_nodes; node_id++) {
        if (data_distribution[node_id] > _code.lambda_f) {
            data_relocation_cost += (int) (data_distribution[node_id] - _code.lambda_f);
        }
    }

    return data_relocation_cost;
}

int StripeGroup::getMinParityMergingCost() {

    // parity merging cost
    if (_code.isValidForPM() == false) {
        printf("invalid code for parity merging\n");
        return -1;
    }

    size_t num_nodes = _settings.M;
    
    vector<size_t> data_distribution = getDataDistribution();
    vector<vector<size_t> > parity_distributions = getParityDistributions();
    
    int total_pm_cost = 0;

    vector<size_t> selected_nodes;
    
    for (size_t parity_id = 0; parity_id < _code.m_f; parity_id++) {
        // candidate nodes for parity merging
        vector<size_t> &parity_distribution = parity_distributions[parity_id];
        vector<int> pm_costs(num_nodes, 0);

        for (size_t node_id = 0; node_id < num_nodes; node_id++) {
            pm_costs[node_id] = (int) (_code.alpha - parity_distribution[node_id]);
            // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
            if (data_distribution[node_id] > 0) {
                pm_costs[node_id] += 1;
            }
        }

        // find minimum cost among all nodes
        int min_pm_cost = *min_element(pm_costs.begin(), pm_costs.end());
        total_pm_cost += min_pm_cost;
    }

    return total_pm_cost;
}

int StripeGroup::getMinReEncodingCost() {

    vector<size_t> data_distribution = getDataDistribution();

    int total_re_cost = 0;

    // sort number of data blocks by descending order
    vector<size_t> sorted_idx = Utils::argsortUIntVector(data_distribution);
    reverse(sorted_idx.begin(), sorted_idx.end());

    // it targets at general parameters (for each of lambda_f converted stripes, we should collect k_f data blocks to a node, no matter where they come from)
    for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++) {
        // required number of data blocks at the compute node
        size_t num_data_required = _code.k_f - data_distribution[sorted_idx[final_sid]];

        // number of parity blocks that needs relocation from the compute node
        size_t num_parity_reloc = _code.m_f;

        total_re_cost += (int) (num_data_required + num_parity_reloc);
        
        // printf("num_data_required: %d, num_parity_reloc: %d\n", num_data_required, num_parity_reloc);
    }

    return total_re_cost;
}

vector<vector<size_t> > StripeGroup::getCandSendLoadTablesForMinTransCost(int min_cost) {

    vector<vector<size_t> > cand_send_load_tables;

    size_t num_nodes = _settings.M;
    vector<size_t> init_send_load_table(num_nodes, 0);

    // data relocation cost
    int data_reloc_cost = constructSendLoadTableWithDataRelocation(init_send_load_table);

    // approach 1: re-encoding cost (baseline)
    appendSendLoadTableWithReEncoding(init_send_load_table, cand_send_load_tables, min_cost - data_reloc_cost);

    if (_code.isValidForPM() == true) {
        // approach 2: parity merging
        appendSendLoadTableWithParityMerging(init_send_load_table, cand_send_load_tables, min_cost - data_reloc_cost);
    }

    return cand_send_load_tables;
}

int StripeGroup::constructSendLoadTableWithDataRelocation(vector<size_t> &send_load_table) {
    size_t num_nodes = _settings.M;

    int data_relocation_cost = 0;
    vector<size_t> data_distribution = getDataDistribution();

    // printf("data_distribution:\n");
    // Utils::printIntVector(data_distribution);

    // it targets at general parameters (meaning that for a converted group, there must exist no more than lambda_f data blocks in a node)
    for (size_t node_id = 0; node_id < num_nodes; node_id++) {
        if (data_distribution[node_id] > _code.lambda_f) {
            size_t num_data_blocks_to_reloc = data_distribution[node_id] - _code.lambda_f;

            data_relocation_cost += (int) num_data_blocks_to_reloc;

            // update send load table
            send_load_table[node_id] += num_data_blocks_to_reloc;
        }
    }

    return data_relocation_cost;
}

int StripeGroup::appendSendLoadTableWithParityMerging(vector<size_t> &init_send_load_table, vector<vector<size_t> > &cand_send_load_tables, int min_cost) {

    // parity merging cost
    if (_code.isValidForPM() == false) {
        printf("invalid code for parity merging\n");
        return -1;
    }

    size_t num_nodes = _settings.M;

    // candidate load table
    vector<size_t> cand_send_load_table = init_send_load_table;
    
    vector<size_t> data_distribution = getDataDistribution();
    vector<vector<size_t> > parity_distributions = getParityDistributions();
    
    int total_pm_cost = 0;

    vector<size_t> selected_nodes;
    
    for (size_t parity_id = 0; parity_id < _code.m_f; parity_id++) {
        // candidate nodes for parity merging
        vector<size_t> &parity_distribution = parity_distributions[parity_id];
        vector<int> pm_costs(num_nodes, 0);

        for (size_t node_id = 0; node_id < num_nodes; node_id++) {
            pm_costs[node_id] = (int) (_code.alpha - parity_distribution[node_id]);
            // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
            if (data_distribution[node_id] > 0) {
                pm_costs[node_id] += 1;
            }
        }

        // find minimum cost among all nodes
        int min_pm_cost = *min_element(pm_costs.begin(), pm_costs.end());

        int min_pm_cost_node_id = distance(pm_costs.begin(), min_element(pm_costs.begin(), pm_costs.end()));

        for (size_t node_id = 0; node_id < num_nodes; node_id++) {
            if (parity_distribution[node_id] > 0 && node_id != min_pm_cost_node_id) {
                cand_send_load_table[node_id] += parity_distribution[node_id];
            }
        }
        // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
        if (data_distribution[min_pm_cost_node_id] > 0) {
            cand_send_load_table[min_pm_cost_node_id] += 1;
        }

        total_pm_cost += min_pm_cost;
    }

    cand_send_load_tables.push_back(cand_send_load_table);

    return total_pm_cost;
}


int StripeGroup::appendSendLoadTableWithReEncoding(vector<size_t> &init_send_load_table, vector<vector<size_t> > &cand_send_load_tables, int min_cost) {

    size_t num_nodes = _settings.M;
    vector<size_t> data_distribution = getDataDistribution();

    int total_re_cost = 0;

    // sort number of data blocks by descending order
    vector<size_t> sorted_idx = Utils::argsortUIntVector(data_distribution);
    reverse(sorted_idx.begin(), sorted_idx.end());

    // it targets at general parameters (for each of lambda_f converted stripes, we should collect k_f data blocks to a node, no matter where they come from)
    for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++) {
        // required number of data blocks at the compute node
        size_t num_data_required = _code.k_f - data_distribution[sorted_idx[final_sid]];

        // number of parity blocks that needs relocation from the compute node
        size_t num_parity_reloc = _code.m_f;

        total_re_cost += (int) (num_data_required + num_parity_reloc);
        
        // printf("num_data_required: %d, num_parity_reloc: %d\n", num_data_required, num_parity_reloc);
    }

    if (total_re_cost <= min_cost) {
        vector<size_t> cand_send_load_table = init_send_load_table;

        for (size_t idx = 0; idx < num_nodes; idx++) {
            size_t sorted_node_id = sorted_idx[idx];
            if (idx < _code.lambda_f) {
                // number of parity blocks that needs relocation from the compute node
                size_t num_parity_reloc = _code.m_f;

                cand_send_load_table[sorted_node_id] += num_parity_reloc;
            } else {
                // required number of data blocks at the compute node
                size_t num_data_to_send = data_distribution[sorted_node_id];

                cand_send_load_table[sorted_node_id] += num_data_to_send;
            }
        }

        // append the candidate send load table to list
        cand_send_load_tables.push_back(cand_send_load_table);
    }

    return total_re_cost;
}