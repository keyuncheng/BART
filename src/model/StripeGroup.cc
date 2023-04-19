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

vector<size_t> StripeGroup::getDataDistribution()
{
    size_t num_nodes = _settings.M;

    vector<size_t> data_distribution(num_nodes, 0);

    for (auto stripe : _stripes)
    {
        vector<size_t> &stripe_indices = stripe->getStripeIndices();

        for (size_t block_id = 0; block_id < _code.k_i; block_id++)
        {
            data_distribution[stripe_indices[block_id]] += 1;
        }
    }

    return data_distribution;
}

vector<vector<size_t>> StripeGroup::getParityDistributions()
{
    vector<vector<size_t>> parity_distributions;

    for (size_t parity_id = 0; parity_id < _code.m_i; parity_id++)
    {
        parity_distributions.push_back(getParityDistribution(parity_id));
    }

    return parity_distributions;
}

vector<size_t> StripeGroup::getParityDistribution(size_t parity_id)
{
    size_t num_nodes = _settings.M;
    vector<size_t> parity_distribution(num_nodes, 0);

    // check which node already stores at least one data block
    for (auto stripe : _stripes)
    {
        vector<size_t> &stripe_indices = stripe->getStripeIndices();
        size_t parity_node_id = stripe_indices[_code.k_i + parity_id];
        parity_distribution[parity_node_id] += 1;
    }

    return parity_distribution;
}

ConvertibleCode &StripeGroup::getCode()
{
    return _code;
}

ClusterSettings &StripeGroup::getClusterSettings()
{
    return _settings;
}

vector<Stripe *> &StripeGroup::getStripes()
{
    return _stripes;
}

size_t StripeGroup::getId()
{
    return _id;
}

void StripeGroup::print()
{
    printf("Stripe group %ld:\n", _id);
    for (auto stripe : _stripes)
    {
        stripe->print();
    }
}

int StripeGroup::getMinTransitionCost()
{

    // data relocation cost
    int data_reloc_cost = getDataRelocationCost();
    int parity_update_cost = 0;

    // approach 1: re-encoding cost (baseline)
    int re_cost = getMinReEncodingCost();
    parity_update_cost = re_cost;

    int pm_cost = -1;

    if (_code.isValidForPM() == true)
    {
        // approach 2: parity merging
        pm_cost = getMinParityMergingCost();
        if (pm_cost < re_cost)
        {
            parity_update_cost = pm_cost;
        }
    }

    // printf("data_relocation_cost: %d, parity_update_cost: %d, re-encoding cost: %d, parity merging cost: %d\n", data_reloc_cost, parity_update_cost, re_cost, pm_cost);

    return data_reloc_cost + parity_update_cost;
}

int StripeGroup::getDataRelocationCost()
{
    size_t num_nodes = _settings.M;

    int data_relocation_cost = 0;
    vector<size_t> data_distribution = getDataDistribution();

    // printf("data_distribution:\n");
    // Utils::printIntVector(data_distribution);

    // it targets at general parameters (meaning that for a converted group, there must exist no more than lambda_f data blocks in a node)
    for (size_t node_id = 0; node_id < num_nodes; node_id++)
    {
        if (data_distribution[node_id] > _code.lambda_f)
        {
            data_relocation_cost += (int)(data_distribution[node_id] - _code.lambda_f);
        }
    }

    return data_relocation_cost;
}

int StripeGroup::getMinParityMergingCost()
{

    // parity merging cost
    if (_code.isValidForPM() == false)
    {
        printf("invalid code for parity merging\n");
        return -1;
    }

    size_t num_nodes = _settings.M;

    vector<size_t> data_distribution = getDataDistribution();
    vector<vector<size_t>> parity_distributions = getParityDistributions();

    int total_pm_cost = 0;

    vector<size_t> selected_nodes;

    for (size_t parity_id = 0; parity_id < _code.m_f; parity_id++)
    {
        // candidate nodes for parity merging
        vector<size_t> &parity_distribution = parity_distributions[parity_id];
        vector<int> pm_costs(num_nodes, 0);

        for (size_t node_id = 0; node_id < num_nodes; node_id++)
        {
            pm_costs[node_id] = (int)(_code.alpha - parity_distribution[node_id]);
            // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
            if (data_distribution[node_id] > 0)
            {
                pm_costs[node_id] += 1;
            }
        }

        // find minimum cost among all nodes
        int min_pm_cost = *min_element(pm_costs.begin(), pm_costs.end());
        total_pm_cost += min_pm_cost;
    }

    return total_pm_cost;
}

int StripeGroup::getMinReEncodingCost()
{

    vector<size_t> data_distribution = getDataDistribution();

    int total_re_cost = 0;

    // sort number of data blocks by descending order
    vector<size_t> sorted_idx = Utils::argsortUIntVector(data_distribution);
    reverse(sorted_idx.begin(), sorted_idx.end());

    // it targets at general parameters (for each of lambda_f converted stripes, we should collect k_f data blocks to a node, no matter where they come from)
    for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++)
    {
        // required number of data blocks at the compute node
        size_t num_data_required = _code.k_f - data_distribution[sorted_idx[final_sid]];

        // number of parity blocks that needs relocation from the compute node
        size_t num_parity_reloc = _code.m_f;

        total_re_cost += (int)(num_data_required + num_parity_reloc);

        // printf("num_data_required: %d, num_parity_reloc: %d\n", num_data_required, num_parity_reloc);
    }

    return total_re_cost;
}

vector<vector<size_t>> StripeGroup::getCandSendLoadTables()
{
    size_t num_nodes = _settings.M;

    // candidate send load tables
    vector<vector<size_t>> cand_slts;

    // construct initial send load table with data relocation
    vector<size_t> init_slt(num_nodes, 0); // initial send load table
    constructInitSLTWithDataRelocation(init_slt);

    // printf("stripe group: %ld, initial send load table:\n", _id);
    // Utils::printUIntVector(init_slt);

    // approach 1: re-encoding
    appendCandSLTsWithReEncoding(init_slt, cand_slts);

    if (_code.isValidForPM() == true)
    {
        // approach 2: parity merging
        appendCandSLTsWithParityMerging(init_slt, cand_slts);
    }

    return cand_slts;
}

int StripeGroup::constructInitSLTWithDataRelocation(vector<size_t> &send_load_table)
{
    size_t num_nodes = _settings.M;

    int data_relocation_cost = 0;
    vector<size_t> data_distribution = getDataDistribution();

    // printf("data_distribution:\n");
    // Utils::printIntVector(data_distribution);

    // it targets at general parameters (meaning that for a converted group, there must exist no more than lambda_f data blocks in a node)
    for (size_t node_id = 0; node_id < num_nodes; node_id++)
    {
        if (data_distribution[node_id] > _code.lambda_f)
        {
            size_t num_data_blocks_to_reloc = data_distribution[node_id] - _code.lambda_f;

            data_relocation_cost += (int)num_data_blocks_to_reloc;

            // update send load table
            send_load_table[node_id] += num_data_blocks_to_reloc;
        }
    }

    return data_relocation_cost;
}

void StripeGroup::appendCandSLTsWithParityMerging(vector<size_t> &init_slt, vector<vector<size_t>> &cand_slts)
{
    /**
     * for parity merging, lambda_f = 1;
     * select distinct m_f nodes to do parity merging; there are comb(num_nodes, m_f) such cases
     * the send load distribution for each stripe:
     * (1) parity block collection: for each of m_f parity blocks, collect lambda_i parity blocks with the corresponding index to the selected node
     * (2) new parity block relocation: if there is a data block on the selected node, need to send the newly computed block to other node
     *
     **/

    // check parameter for parity merging
    if (_code.isValidForPM() == false)
    {
        printf("invalid code for parity merging\n");
        return;
    }

    size_t num_nodes = _settings.M;

    vector<size_t> data_distribution = getDataDistribution();
    vector<vector<size_t>> parity_distributions = getParityDistributions();

    vector<vector<size_t>> combs = Utils::getCombinations(num_nodes, _code.m_f);

    for (size_t comb_id = 0; comb_id < combs.size(); comb_id++)
    {
        // list the selected m_f nodes to do parity computation
        vector<size_t> &comb = combs[comb_id];

        // there are m_f! possible choices to do computation
        do
        {
            // create a copy of candidate send load table
            vector<size_t> cand_slt = init_slt;
            for (size_t parity_id = 0; parity_id < _code.m_f; parity_id++)
            {
                vector<size_t> &parity_distribution = parity_distributions[parity_id];
                size_t parity_comp_node = comb[parity_id];

                // (1) parity block collection
                for (size_t node_id = 0; node_id < num_nodes; node_id++)
                {
                    // send the parity block
                    if (node_id != parity_comp_node)
                    {
                        cand_slt[node_id] += parity_distribution[node_id];
                    }
                }
                // (2) new parity block relocation
                if (data_distribution[parity_comp_node] > 0)
                {
                    cand_slt[parity_comp_node] += 1;
                }
            }

            // add candidate send load table
            cand_slts.push_back(cand_slt);

        } while (next_permutation(comb.begin(), comb.end()));
    }

    return;
}

void StripeGroup::appendCandSLTsWithReEncoding(vector<size_t> &init_slt, vector<vector<size_t>> &cand_slts)
{
    /**
     * for each of lambda_f stripes, find the node to do re-encoding, and relocate the m_f new parity blocks to other nodes. There are perm(num_nodes, lambda_f) such cases.
     * the send load distribution for each stripe:
     * (1) data block collection: all nodes except the selected nodes send all data blocks
     * (2) parity block relocation: the selected lambda_f nodes each sends m_f parity blocks
     *
     **/

    size_t num_nodes = _settings.M;
    vector<size_t> data_distribution = getDataDistribution();

    // first, get all permutations of (M, lambda_f) selected nodes to do re-encoding
    vector<vector<size_t>> perms = Utils::getPermutation(_code.lambda_f, num_nodes);

    for (size_t perm_id = 0; perm_id < perms.size(); perm_id++)
    {
        // create a copy of init send load table
        vector<size_t> cand_slt = init_slt;

        vector<size_t> &perm = perms[perm_id];

        // mark the nodes for parity computation
        vector<bool> is_parity_comp_node(num_nodes, false);
        for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++)
        {
            is_parity_comp_node[perm[final_sid]] = true;
        }

        // send load type 1: data block collection
        for (size_t node_id = 0; node_id < num_nodes; node_id++)
        {
            // send all data blocks in the node
            if (is_parity_comp_node[node_id] == false)
            {
                cand_slt[node_id] += data_distribution[node_id];
            }
        }

        // send load type 2: parity distribution
        for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++)
        {
            // send m_f parity blocks from the node
            size_t parity_comp_node_id = perm[final_sid];
            cand_slt[parity_comp_node_id] += _code.m_f;
        }

        // append the candidate send load table to list
        cand_slts.push_back(cand_slt);
    }

    return;
}

vector<vector<size_t>> StripeGroup::getCandSendLoadTablesForMinTransCost(int min_cost)
{

    vector<vector<size_t>> cand_send_load_tables;

    size_t num_nodes = _settings.M;
    vector<size_t> init_send_load_table(num_nodes, 0);

    // data relocation cost
    int data_reloc_cost = constructInitSLTWithDataRelocation(init_send_load_table);

    // approach 1: re-encoding cost (baseline)
    appendMinCostSLTWithReEncoding(init_send_load_table, cand_send_load_tables, min_cost - data_reloc_cost);

    if (_code.isValidForPM() == true)
    {
        // approach 2: parity merging
        appendMinCostSLTWithParityMerging(init_send_load_table, cand_send_load_tables, min_cost - data_reloc_cost);
    }

    return cand_send_load_tables;
}

int StripeGroup::appendMinCostSLTWithParityMerging(vector<size_t> &init_send_load_table, vector<vector<size_t>> &cand_send_load_tables, int min_cost)
{

    // parity merging cost
    if (_code.isValidForPM() == false)
    {
        printf("invalid code for parity merging\n");
        return -1;
    }

    size_t num_nodes = _settings.M;

    // candidate load table
    vector<size_t> cand_send_load_table = init_send_load_table;

    vector<size_t> data_distribution = getDataDistribution();
    vector<vector<size_t>> parity_distributions = getParityDistributions();

    int total_pm_cost = 0;

    for (size_t parity_id = 0; parity_id < _code.m_f; parity_id++)
    {
        // candidate nodes for parity merging
        vector<size_t> &parity_distribution = parity_distributions[parity_id];
        vector<int> pm_costs(num_nodes, 0);

        for (size_t node_id = 0; node_id < num_nodes; node_id++)
        {
            pm_costs[node_id] = (int)(_code.alpha - parity_distribution[node_id]);
            // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
            if (data_distribution[node_id] > 0)
            {
                pm_costs[node_id] += 1;
            }
        }

        // find minimum cost among all nodes
        int min_pm_cost = *min_element(pm_costs.begin(), pm_costs.end());

        size_t min_pm_cost_node_id = distance(pm_costs.begin(), min_element(pm_costs.begin(), pm_costs.end()));

        for (size_t node_id = 0; node_id < num_nodes; node_id++)
        {
            if (parity_distribution[node_id] > 0 && node_id != min_pm_cost_node_id)
            {
                cand_send_load_table[node_id] += parity_distribution[node_id];
            }
        }
        // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
        if (data_distribution[min_pm_cost_node_id] > 0)
        {
            cand_send_load_table[min_pm_cost_node_id] += 1;
        }

        total_pm_cost += min_pm_cost;
    }

    cand_send_load_tables.push_back(cand_send_load_table);

    return total_pm_cost;
}

int StripeGroup::appendMinCostSLTWithReEncoding(vector<size_t> &init_send_load_table, vector<vector<size_t>> &cand_send_load_tables, int min_cost)
{

    size_t num_nodes = _settings.M;
    vector<size_t> data_distribution = getDataDistribution();

    int total_re_cost = 0;

    // sort number of data blocks by descending order
    vector<size_t> sorted_idx = Utils::argsortUIntVector(data_distribution);
    reverse(sorted_idx.begin(), sorted_idx.end());

    // it targets at general parameters (for each of lambda_f converted stripes, we should collect k_f data blocks to a node, no matter where they come from)
    for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++)
    {
        // required number of data blocks at the compute node
        size_t num_data_required = _code.k_f - data_distribution[sorted_idx[final_sid]];

        // number of parity blocks that needs relocation from the compute node
        size_t num_parity_reloc = _code.m_f;

        total_re_cost += (int)(num_data_required + num_parity_reloc);

        // printf("num_data_required: %d, num_parity_reloc: %d\n", num_data_required, num_parity_reloc);
    }

    if (total_re_cost <= min_cost)
    {
        vector<size_t> cand_send_load_table = init_send_load_table;

        for (size_t idx = 0; idx < num_nodes; idx++)
        {
            size_t sorted_node_id = sorted_idx[idx];
            if (idx < _code.lambda_f)
            {
                // number of parity blocks that needs relocation from the compute node
                size_t num_parity_reloc = _code.m_f;

                cand_send_load_table[sorted_node_id] += num_parity_reloc;
            }
            else
            {
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

vector<LoadTable> StripeGroup::getCandSLTs()
{
    size_t num_nodes = _settings.M;

    // candidate send load tables
    vector<LoadTable> cand_slts;

    // construct initial send load table with data relocation
    LoadTable pslt_dr = getPartialSLTWithDataRelocation();

    // send load table of re-encoding
    LoadTable slt_re;
    LoadTable pslt_re = getPartialSLTWithReEncoding();
    slt_re.approach = TransApproach::RE_ENCODE;
    slt_re.lt = Utils::dotAddUIntVectors(pslt_dr.lt, pslt_re.lt);
    slt_re.cost = pslt_dr.cost + pslt_re.cost;

    cand_slts.push_back(slt_re);

    // send load table of parity merging
    if (_code.isValidForPM() == true)
    {
        LoadTable slt_pm;
        LoadTable pslt_pm = getPartialSLTWIthParityMerging();
        slt_pm.lt = Utils::dotAddUIntVectors(pslt_dr.lt, pslt_pm.lt);
        slt_pm.cost = pslt_dr.cost + pslt_pm.cost;

        cand_slts.push_back(slt_re);
    }

    for (auto &cand_slt : cand_slts)
    {
        printf("stripe group: %ld, approach: %ld, cost: %ld, send load table:\n", _id, cand_slt.approach, cand_slt.cost);
        Utils::printUIntVector(cand_slt.lt);
    }

    return cand_slts;
}

LoadTable StripeGroup::getPartialSLTWithDataRelocation()
{
    size_t num_nodes = _settings.M;

    LoadTable pslt_dr;
    pslt_dr.lt = vector<size_t>(num_nodes, 0);

    vector<size_t> data_distribution = getDataDistribution();

    // it targets at general parameters (meaning that for a pre-transitioning group, there must exist no more than lambda_f data blocks in a node)
    for (size_t node_id = 0; node_id < num_nodes; node_id++)
    {
        if (data_distribution[node_id] > _code.lambda_f)
        {
            size_t num_data_blocks_to_reloc = data_distribution[node_id] - _code.lambda_f;

            pslt_dr.cost += num_data_blocks_to_reloc;

            // update send load table
            pslt_dr.lt[node_id] += num_data_blocks_to_reloc;
        }
    }

    return pslt_dr;
}

LoadTable StripeGroup::getPartialSLTWithReEncoding()
{

    size_t num_nodes = _settings.M;
    LoadTable pslt_re;
    vector<size_t> data_distribution = getDataDistribution();

    // sort number of data blocks by descending order
    vector<size_t> sorted_idx = Utils::argsortUIntVector(data_distribution);
    reverse(sorted_idx.begin(), sorted_idx.end());

    // it targets at general parameters (for each of lambda_f converted stripes, we should collect k_f data blocks to a node, no matter where they come from)
    for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++)
    {
        // required number of data blocks at the compute node
        size_t num_data_required = _code.k_f - data_distribution[sorted_idx[final_sid]];

        // number of parity blocks that needs relocation from the compute node
        size_t num_parity_reloc = _code.m_f;

        total_re_cost += (int)(num_data_required + num_parity_reloc);

        // printf("num_data_required: %d, num_parity_reloc: %d\n", num_data_required, num_parity_reloc);
    }

    if (total_re_cost <= min_cost)
    {
        vector<size_t> cand_send_load_table = init_send_load_table;

        for (size_t idx = 0; idx < num_nodes; idx++)
        {
            size_t sorted_node_id = sorted_idx[idx];
            if (idx < _code.lambda_f)
            {
                // number of parity blocks that needs relocation from the compute node
                size_t num_parity_reloc = _code.m_f;

                cand_send_load_table[sorted_node_id] += num_parity_reloc;
            }
            else
            {
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

LoadTable StripeGroup::getPartialSLTWIthParityMerging()
{
    // parity merging cost
    if (_code.isValidForPM() == false)
    {
        printf("invalid code for parity merging\n");
        return -1;
    }

    size_t num_nodes = _settings.M;

    // candidate load table
    vector<size_t> cand_send_load_table = init_send_load_table;

    vector<size_t> data_distribution = getDataDistribution();
    vector<vector<size_t>> parity_distributions = getParityDistributions();

    int total_pm_cost = 0;

    for (size_t parity_id = 0; parity_id < _code.m_f; parity_id++)
    {
        // candidate nodes for parity merging
        vector<size_t> &parity_distribution = parity_distributions[parity_id];
        vector<int> pm_costs(num_nodes, 0);

        for (size_t node_id = 0; node_id < num_nodes; node_id++)
        {
            pm_costs[node_id] = (int)(_code.alpha - parity_distribution[node_id]);
            // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
            if (data_distribution[node_id] > 0)
            {
                pm_costs[node_id] += 1;
            }
        }

        // find minimum cost among all nodes
        int min_pm_cost = *min_element(pm_costs.begin(), pm_costs.end());

        size_t min_pm_cost_node_id = distance(pm_costs.begin(), min_element(pm_costs.begin(), pm_costs.end()));

        for (size_t node_id = 0; node_id < num_nodes; node_id++)
        {
            if (parity_distribution[node_id] > 0 && node_id != min_pm_cost_node_id)
            {
                cand_send_load_table[node_id] += parity_distribution[node_id];
            }
        }
        // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
        if (data_distribution[min_pm_cost_node_id] > 0)
        {
            cand_send_load_table[min_pm_cost_node_id] += 1;
        }

        total_pm_cost += min_pm_cost;
    }

    cand_send_load_tables.push_back(cand_send_load_table);

    return total_pm_cost;
}
