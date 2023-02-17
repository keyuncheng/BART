#include "StripeGroup.hh"

StripeGroup::StripeGroup(int id, ConvertibleCode &code, ClusterSettings &settings, vector<Stripe *> &stripes)
{
    _id = id;
    _code = code;
    _settings = settings;
    _stripes = stripes;
}

StripeGroup::~StripeGroup()
{
}

vector<int> StripeGroup::getDataDistribution() {
    int num_nodes = _settings.M;

    vector<int> data_distribution(num_nodes, 0);

    for (auto stripe : _stripes) {
        vector<int> &stripe_indices = stripe->getStripeIndices();

        for (int block_id = 0; block_id < _code.k_i; block_id++) {
            data_distribution[stripe_indices[block_id]] += 1;
        }
    }

    return data_distribution;
}

vector<vector<int> > StripeGroup::getParityDistributions() {
    int num_nodes = _settings.M;

    vector<vector<int> > parity_distributions;

    for (int parity_id = 0; parity_id < _code.m_i; parity_id++) {
        vector<int> parity_distribution(num_nodes, 0);

        // check which node already stores at least one data block
        for (auto stripe : _stripes) {
            vector<int> &stripe_indices = stripe->getStripeIndices();
            int parity_node_id = stripe_indices[_code.k_i + parity_id];
            parity_distribution[parity_node_id] += 1;
        }

        parity_distributions.push_back(parity_distribution);
    }

    return parity_distributions;
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

int StripeGroup::getId() {
    return _id;
}

void StripeGroup::print() {
    printf("Stripe group %d:\n", _id);
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
    int num_nodes = _settings.M;

    int data_relocation_cost = 0;
    vector<int> data_distribution = getDataDistribution();

    // printf("data_distribution:\n");
    // Utils::printIntVector(data_distribution);

    // it targets at general parameters (meaning that for a converted group, there must exist no more than lambda_f data blocks in a node)
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (data_distribution[node_id] > _code.lambda_f) {
            data_relocation_cost += data_distribution[node_id] - _code.lambda_f;
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

    int num_nodes = _settings.M;
    
    vector<int> data_distribution = getDataDistribution();
    vector<vector<int> > parity_distributions = getParityDistributions();
    
    int total_pm_cost = 0;
    
    for (int parity_id = 0; parity_id < _code.m_f; parity_id++) {
        // candidate nodes for parity merging
        vector<int> &parity_distribution = parity_distributions[parity_id];
        vector<int> pm_costs(num_nodes, 0);

        for (int node_id = 0; node_id < num_nodes; node_id++) {
            pm_costs[node_id] = _code.alpha - parity_distribution[node_id];
            // the node already stores a data block, need to relocate to another node for merging
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

    vector<int> data_distribution = getDataDistribution();

    int total_re_cost = 0;

    // sort number of data blocks by descending order
    vector<int> sorted_idx = Utils::argsortIntVector(data_distribution);
    reverse(sorted_idx.begin(), sorted_idx.end());

    // it targets at general parameters (for each of lambda_f converted stripes, we should collect k_f data blocks to a node, no matter where they come from)
    for (int final_sid = 0; final_sid < _code.lambda_f; final_sid++) {
        // required number of data blocks at the compute node
        int num_data_required = _code.k_f - data_distribution[sorted_idx[final_sid]];


        // number of parity blocks that needs relocation from the compute node
        int num_parity_reloc = _code.m_f;

        total_re_cost += num_data_required + num_parity_reloc;
        
        // printf("num_data_required: %d, num_parity_reloc: %d\n", num_data_required, num_parity_reloc);
    }

    return total_re_cost;
}