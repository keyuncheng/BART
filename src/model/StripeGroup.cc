#include "StripeGroup.hh"

StripeGroup::StripeGroup(uint32_t _id, ConvertibleCode &_code, ClusterSettings &_settings, vector<Stripe *> &_sg_stripes) : id(_id), code(_code), settings(_settings), sg_stripes(_sg_stripes)
{
    initDataDist();
    initParityDists();
}

StripeGroup::~StripeGroup()
{
}

void StripeGroup::print()
{
    printf("Stripe group %u: [", id);
    for (auto stripe : sg_stripes)
    {
        printf("%u, ", stripe->id);
    }
    printf("]\n");

    for (auto stripe : sg_stripes)
    {
        stripe->print();
    }
}

void StripeGroup::initDataDist()
{

    // init data distribution
    data_dist.assign(settings.num_nodes, 0);

    for (auto &stripe : sg_stripes)
    {
        for (uint8_t block_id = 0; block_id < code.k_i; block_id++)
        {
            data_dist[stripe->indices[block_id]] += 1;
        }
    }
}

void StripeGroup::initParityDists()
{

    // init data distribution
    parity_dists.clear();
    parity_dists.assign(code.m_i, u16string(settings.num_nodes, 0));

    for (uint8_t parity_id = 0; parity_id < code.m_i; parity_id++)
    {
        for (auto &stripe : sg_stripes)
        {
            uint16_t parity_node_id = stripe->indices[code.k_i + parity_id];
            parity_dists[parity_id][parity_node_id] += 1;
        }
    }
}

uint8_t StripeGroup::getMinTransBW()
{
    uint8_t parity_update_bw = 0;
    // // approach 1: re-encoding bandwidth
    // uint8_t re_bw = getMinREBW();
    // parity_update_bw = re_bw;

    // // approach 2: parity merging
    // if (code.isValidForPM() == true)
    // {
    //     uint8_t pm_bw = getMinPMBW();

    //     if (pm_bw <= re_bw)
    //     {
    //         parity_update_bw = pm_bw;
    //     }
    // }

    // NOTE: here we assume that bandwidth(pm) <= bandwith (re), thus we calculate pm bandwidth only
    parity_update_bw = getMinPMBW();

    // data relocation cost
    uint8_t data_reloc_bw = getDataRelocBW();

    return parity_update_bw + data_reloc_bw;
}

uint8_t StripeGroup::getDataRelocBW()
{
    // it targets at general parameters (meaning that for a converted group, there must exist no more than lambda_f data blocks in a node)

    uint8_t data_reloc_bw = 0;

    for (uint16_t node_id = 0; node_id < settings.num_nodes; node_id++)
    {
        if (data_dist[node_id] > code.lambda_f)
        {
            data_reloc_bw += data_dist[node_id] - code.lambda_f;
        }
    }

    return data_reloc_bw;
}

uint8_t StripeGroup::getMinREBW()
{
    uint8_t re_bw = 0;

    // sort number of data blocks by descending order
    vector<size_t> sorted_idx = Utils::argsortVector(data_dist);
    reverse(sorted_idx.begin(), sorted_idx.end());

    // it targets at general parameters (for each of lambda_f converted stripes, we should collect k_f data blocks to a node, no matter where they come from)
    for (uint8_t final_sid = 0; final_sid < code.lambda_f; final_sid++)
    {
        // parity generation bw + parity relocation bw
        re_bw += (code.k_f - data_dist[sorted_idx[final_sid]]) + code.m_f;
    }

    return re_bw;
}

uint8_t StripeGroup::getMinPMBW()
{
    uint16_t num_nodes = settings.num_nodes;
    // for parity merging, there are <num_nodes ^ code.m_f> possible choices to compute parity blocks, as we can collect each of m_f parity blocks at num_nodes nodes
    uint32_t num_pm_choices = pow(num_nodes, code.m_f);

    uint8_t min_bw = UINT8_MAX; // record minimum bw
    // enumerate m_f nodes for parity merging
    u16string pm_nodes(code.m_f, 0); // computation for parity i is at pm_nodes[i]
    for (uint32_t perm_id = 0; perm_id < num_pm_choices; perm_id++)
    {
        u16string relocated_nodes = data_dist; // mark the number of blocks relocated on the node
        uint8_t cur_perm_bw = 0;               // record current bw for perm_id
        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            u16string &parity_dist = parity_dists[parity_id];
            uint16_t parity_compute_node = pm_nodes[parity_id];

            uint8_t pm_bw = (code.alpha - parity_dist[parity_compute_node]) + (relocated_nodes[parity_compute_node] > 0 ? 1 : 0); // required number of parity blocks + parity relocation bw

            relocated_nodes[parity_compute_node] += 1; // mark the node as relocated on the node
            cur_perm_bw += pm_bw;                      // update bw
        }

        // update minimum bw
        if (cur_perm_bw < min_bw)
        {
            min_bw = cur_perm_bw;
        }

        // get next permutation
        Utils::getNextPerm(num_nodes, code.m_f, pm_nodes);
    }

    return min_bw;
}

uint8_t StripeGroup::getMinPMBWGreedy()
{
    uint8_t sum_pm_bw = 0;
    u16string relocated_nodes = data_dist; // mark if nodes are relocated

    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        // candidate nodes for parity merging
        u16string &parity_dist = parity_dists[parity_id];

        uint16_t min_bw_node = UINT16_MAX;
        uint8_t min_bw = UINT8_MAX;

        for (uint16_t node_id = 0; node_id < settings.num_nodes; node_id++)
        {
            // parity generation bw + parity relocation bw (check if the node already stores a block)
            uint8_t pm_bw = (code.alpha - parity_dist[node_id]) + (relocated_nodes[node_id] > 0 ? 1 : 0);

            if (pm_bw < min_bw)
            {
                min_bw = pm_bw;
                min_bw_node = node_id;

                // no need to search if min_bw = 0
                if (min_bw == 0)
                {
                    break;
                }
            }
        }

        relocated_nodes[min_bw_node] += 1; // mark the node as relocated
        sum_pm_bw += min_bw;               // update bw
    }

    return sum_pm_bw;
}

void StripeGroup::genParityGenScheme4PerfectPM()
{
    // for perfect parity merging, the partial load table contains send load of data block distribution only
    applied_lt.approach = EncodeMethod::PARITY_MERGE;

    // find corresponding code.m_f nodes for perfect parity merging
    applied_lt.enc_nodes.assign(code.m_f, INVALID_NODE_ID);
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        u16string &parity_dist = parity_dists[parity_id];
        uint16_t min_bw_node_id = distance(parity_dist.begin(), max_element(parity_dist.begin(), parity_dist.end()));
        applied_lt.enc_nodes[parity_id] = min_bw_node_id;
    }
}

void StripeGroup::genAllPartialLTs4ParityGen()
{
    uint16_t num_nodes = settings.num_nodes;
    // for re-encoding, there are <num_nodes> possible candidate load tables, as we can collect code.k_f data blocks and distribute code.m_f parity blocks at <num_nodes> possible nodes
    uint32_t num_re_lts = num_nodes;
    // for parity merging, there are <num_nodes ^ code.m_f> possible candidate load tables, as we can collect each of m_f parity blocks at num_nodes nodes
    uint32_t num_pm_lts = pow(num_nodes, code.m_f);

    // create load tables
    cand_partial_lts.clear();
    cand_partial_lts.resize(num_re_lts + num_pm_lts);

    // enumerate partial load tables for re-encoding
    uint16_t lt_id = 0;
    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        LoadTable &lt = cand_partial_lts[lt_id];
        lt.approach = EncodeMethod::RE_ENCODE;
        lt.bw = code.k_f - data_dist[node_id] + code.m_f;
        lt.enc_nodes.assign(code.m_f, node_id); // re-encoding nodes
        lt.slt = data_dist;                     // send load table
        lt.slt[node_id] = code.m_f;             // only need to send the data blocks at <node_id>
        lt.rlt.assign(num_nodes, 0);            // recv load table
        lt.rlt[node_id] = code.k_f - data_dist[node_id];
        lt.bw = accumulate(lt.slt.begin(), lt.slt.end(), 0); // update bandwidth (for send load)

        lt_id++;
    }

    // enumerate partial load tables for parity merging
    u16string pm_nodes(code.m_f, 0); // computation for parity i is at pm_nodes[i]
    for (uint32_t perm_id = 0; perm_id < num_pm_lts; perm_id++)
    {
        LoadTable &lt = cand_partial_lts[lt_id];
        lt.approach = EncodeMethod::PARITY_MERGE;
        lt.enc_nodes = pm_nodes;     // re-encoding node
        lt.slt.assign(num_nodes, 0); // send load table
        lt.rlt.assign(num_nodes, 0); // recv load table

        // obtain the load distribution for parity blocks
        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            uint16_t parity_comp_node_id = pm_nodes[parity_id];
            // send load dist
            u16string parity_slt = parity_dists[parity_id];
            parity_slt[parity_comp_node_id] = (data_dist[parity_comp_node_id] == 0) ? 0 : 1; // compute at pm_nodes; if there is a data block located there, then need to relocate the generated parity block

            // receive load dist
            u16string parity_rlt(num_nodes, 0);
            parity_rlt[parity_comp_node_id] = code.lambda_i - parity_dists[parity_id][parity_comp_node_id]; // number of required parity block for parity generation

            // accumulate the loads for all parities
            Utils::dotAddVectors(lt.slt, parity_slt, lt.slt);
            Utils::dotAddVectors(lt.rlt, parity_rlt, lt.rlt);
        }

        lt.bw = accumulate(lt.slt.begin(), lt.slt.end(), 0); // update bandwidth (for send load)

        lt_id++;

        // get next permutation
        Utils::getNextPerm(num_nodes, code.m_f, pm_nodes);
    }
}

// vector<vector<size_t>> StripeGroup::getCandSendLoadTables()
// {
//     size_t num_nodes = _settings.M;

//     // candidate send load tables
//     vector<vector<size_t>> cand_slts;

//     // construct initial send load table with data relocation
//     vector<size_t> init_slt(num_nodes, 0); // initial send load table
//     constructInitSLTWithDataRelocation(init_slt);

//     // printf("stripe group: %ld, initial send load table:\n", _id);
//     // Utils::printUIntVector(init_slt);

//     // approach 1: re-encoding
//     appendCandSLTsWithReEncoding(init_slt, cand_slts);

//     if (_code.isValidForPM() == true)
//     {
//         // approach 2: parity merging
//         appendCandSLTsWithParityMerging(init_slt, cand_slts);
//     }

//     return cand_slts;
// }

// int StripeGroup::constructInitSLTWithDataRelocation(vector<size_t> &send_load_table)
// {
//     size_t num_nodes = _settings.M;

//     int data_relocation_cost = 0;
//     vector<size_t> data_distribution = getDataDistribution();

//     // printf("data_distribution:\n");
//     // Utils::printIntVector(data_distribution);

//     // it targets at general parameters (meaning that for a converted group, there must exist no more than lambda_f data blocks in a node)
//     for (size_t node_id = 0; node_id < num_nodes; node_id++)
//     {
//         if (data_distribution[node_id] > _code.lambda_f)
//         {
//             size_t num_data_blocks_to_reloc = data_distribution[node_id] - _code.lambda_f;

//             data_relocation_cost += (int)num_data_blocks_to_reloc;

//             // update send load table
//             send_load_table[node_id] += num_data_blocks_to_reloc;
//         }
//     }

//     return data_relocation_cost;
// }

// void StripeGroup::appendCandSLTsWithParityMerging(vector<size_t> &init_slt, vector<vector<size_t>> &cand_slts)
// {
//     /**
//      * for parity merging, lambda_f = 1;
//      * select distinct m_f nodes to do parity merging; there are comb(num_nodes, m_f) such cases
//      * the send load distribution for each stripe:
//      * (1) parity block collection: for each of m_f parity blocks, collect lambda_i parity blocks with the corresponding index to the selected node
//      * (2) new parity block relocation: if there is a data block on the selected node, need to send the newly computed block to other node
//      *
//      **/

//     // check parameter for parity merging
//     if (_code.isValidForPM() == false)
//     {
//         printf("invalid code for parity merging\n");
//         return;
//     }

//     size_t num_nodes = _settings.M;

//     vector<size_t> data_distribution = getDataDistribution();
//     vector<vector<size_t>> parity_distributions = getParityDistributions();

//     vector<vector<size_t>> combs = Utils::getCombinations(num_nodes, _code.m_f);

//     for (size_t comb_id = 0; comb_id < combs.size(); comb_id++)
//     {
//         // list the selected m_f nodes to do parity computation
//         vector<size_t> &comb = combs[comb_id];

//         // there are m_f! possible choices to do computation
//         do
//         {
//             // create a copy of candidate send load table
//             vector<size_t> cand_slt = init_slt;
//             for (size_t parity_id = 0; parity_id < _code.m_f; parity_id++)
//             {
//                 vector<size_t> &parity_distribution = parity_distributions[parity_id];
//                 size_t parity_comp_node = comb[parity_id];

//                 // (1) parity block collection
//                 for (size_t node_id = 0; node_id < num_nodes; node_id++)
//                 {
//                     // send the parity block
//                     if (node_id != parity_comp_node)
//                     {
//                         cand_slt[node_id] += parity_distribution[node_id];
//                     }
//                 }
//                 // (2) new parity block relocation
//                 if (data_distribution[parity_comp_node] > 0)
//                 {
//                     cand_slt[parity_comp_node] += 1;
//                 }
//             }

//             // add candidate send load table
//             cand_slts.push_back(cand_slt);

//         } while (next_permutation(comb.begin(), comb.end()));
//     }

//     return;
// }

// void StripeGroup::appendCandSLTsWithReEncoding(vector<size_t> &init_slt, vector<vector<size_t>> &cand_slts)
// {
//     /**
//      * for each of lambda_f stripes, find the node to do re-encoding, and relocate the m_f new parity blocks to other nodes. There are perm(num_nodes, lambda_f) such cases.
//      * the send load distribution for each stripe:
//      * (1) data block collection: all nodes except the selected nodes send all data blocks
//      * (2) parity block relocation: the selected lambda_f nodes each sends m_f parity blocks
//      *
//      **/

//     size_t num_nodes = _settings.M;
//     vector<size_t> data_distribution = getDataDistribution();

//     // first, get all permutations of (M, lambda_f) selected nodes to do re-encoding
//     vector<vector<size_t>> perms = Utils::getPermutation(_code.lambda_f, num_nodes);

//     for (size_t perm_id = 0; perm_id < perms.size(); perm_id++)
//     {
//         // create a copy of init send load table
//         vector<size_t> cand_slt = init_slt;

//         vector<size_t> &perm = perms[perm_id];

//         // mark the nodes for parity computation
//         vector<bool> is_parity_comp_node(num_nodes, false);
//         for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++)
//         {
//             is_parity_comp_node[perm[final_sid]] = true;
//         }

//         // send load type 1: data block collection
//         for (size_t node_id = 0; node_id < num_nodes; node_id++)
//         {
//             // send all data blocks in the node
//             if (is_parity_comp_node[node_id] == false)
//             {
//                 cand_slt[node_id] += data_distribution[node_id];
//             }
//         }

//         // send load type 2: parity distribution
//         for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++)
//         {
//             // send m_f parity blocks from the node
//             size_t parity_comp_node_id = perm[final_sid];
//             cand_slt[parity_comp_node_id] += _code.m_f;
//         }

//         // append the candidate send load table to list
//         cand_slts.push_back(cand_slt);
//     }

//     return;
// }

// vector<vector<size_t>> StripeGroup::getCandSendLoadTablesForMinTransCost(int min_cost)
// {

//     vector<vector<size_t>> cand_send_load_tables;

//     size_t num_nodes = _settings.M;
//     vector<size_t> init_send_load_table(num_nodes, 0);

//     // data relocation cost
//     int data_reloc_cost = constructInitSLTWithDataRelocation(init_send_load_table);

//     // approach 1: re-encoding cost (baseline)
//     appendMinCostSLTWithReEncoding(init_send_load_table, cand_send_load_tables, min_cost - data_reloc_cost);

//     if (_code.isValidForPM() == true)
//     {
//         // approach 2: parity merging
//         appendMinCostSLTWithParityMerging(init_send_load_table, cand_send_load_tables, min_cost - data_reloc_cost);
//     }

//     return cand_send_load_tables;
// }

// int StripeGroup::appendMinCostSLTWithParityMerging(vector<size_t> &init_send_load_table, vector<vector<size_t>> &cand_send_load_tables, int min_cost)
// {

//     // parity merging cost
//     if (_code.isValidForPM() == false)
//     {
//         printf("invalid code for parity merging\n");
//         return -1;
//     }

//     size_t num_nodes = _settings.M;

//     // candidate load table
//     vector<size_t> cand_send_load_table = init_send_load_table;

//     vector<size_t> data_distribution = getDataDistribution();
//     vector<vector<size_t>> parity_distributions = getParityDistributions();

//     int total_pm_cost = 0;

//     for (size_t parity_id = 0; parity_id < _code.m_f; parity_id++)
//     {
//         // candidate nodes for parity merging
//         vector<size_t> &parity_distribution = parity_distributions[parity_id];
//         vector<int> pm_costs(num_nodes, 0);

//         for (size_t node_id = 0; node_id < num_nodes; node_id++)
//         {
//             pm_costs[node_id] = (int)(_code.alpha - parity_distribution[node_id]);
//             // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
//             if (data_distribution[node_id] > 0)
//             {
//                 pm_costs[node_id] += 1;
//             }
//         }

//         // find minimum cost among all nodes
//         int min_pm_cost = *min_element(pm_costs.begin(), pm_costs.end());

//         size_t min_pm_cost_node_id = distance(pm_costs.begin(), min_element(pm_costs.begin(), pm_costs.end()));

//         for (size_t node_id = 0; node_id < num_nodes; node_id++)
//         {
//             if (parity_distribution[node_id] > 0 && node_id != min_pm_cost_node_id)
//             {
//                 cand_send_load_table[node_id] += parity_distribution[node_id];
//             }
//         }
//         // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
//         if (data_distribution[min_pm_cost_node_id] > 0)
//         {
//             cand_send_load_table[min_pm_cost_node_id] += 1;
//         }

//         total_pm_cost += min_pm_cost;
//     }

//     cand_send_load_tables.push_back(cand_send_load_table);

//     return total_pm_cost;
// }

// int StripeGroup::appendMinCostSLTWithReEncoding(vector<size_t> &init_send_load_table, vector<vector<size_t>> &cand_send_load_tables, int min_cost)
// {

//     size_t num_nodes = _settings.M;
//     vector<size_t> data_distribution = getDataDistribution();

//     int total_re_cost = 0;

//     // sort number of data blocks by descending order
//     vector<size_t> sorted_idx = Utils::argsortUIntVector(data_distribution);
//     reverse(sorted_idx.begin(), sorted_idx.end());

//     // it targets at general parameters (for each of lambda_f converted stripes, we should collect k_f data blocks to a node, no matter where they come from)
//     for (size_t final_sid = 0; final_sid < _code.lambda_f; final_sid++)
//     {
//         // required number of data blocks at the compute node
//         size_t num_data_required = _code.k_f - data_distribution[sorted_idx[final_sid]];

//         // number of parity blocks that needs relocation from the compute node
//         size_t num_parity_reloc = _code.m_f;

//         total_re_cost += (int)(num_data_required + num_parity_reloc);

//         // printf("num_data_required: %d, num_parity_reloc: %d\n", num_data_required, num_parity_reloc);
//     }

//     if (total_re_cost <= min_cost)
//     {
//         vector<size_t> cand_send_load_table = init_send_load_table;

//         for (size_t idx = 0; idx < num_nodes; idx++)
//         {
//             size_t sorted_node_id = sorted_idx[idx];
//             if (idx < _code.lambda_f)
//             {
//                 // number of parity blocks that needs relocation from the compute node
//                 size_t num_parity_reloc = _code.m_f;

//                 cand_send_load_table[sorted_node_id] += num_parity_reloc;
//             }
//             else
//             {
//                 // required number of data blocks at the compute node
//                 size_t num_data_to_send = data_distribution[sorted_node_id];

//                 cand_send_load_table[sorted_node_id] += num_data_to_send;
//             }
//         }

//         // append the candidate send load table to list
//         cand_send_load_tables.push_back(cand_send_load_table);
//     }

//     return total_re_cost;
// }

// vector<LoadTable> StripeGroup::getCandSLTs()
// {
//     // candidate send load tables
//     vector<LoadTable> cand_slts;

//     // construct initial send load table with data relocation
//     LoadTable pslt_dr = getPartialSLTWithDataRelocation();

//     // send load table of re-encoding
//     LoadTable slt_re = getPartialSLTWithReEncoding();

//     slt_re.approach = EncodeMethod::RE_ENCODE;
//     slt_re.lt = Utils::dotAddUIntVectors(slt_re.lt, pslt_dr.lt);
//     slt_re.cost = pslt_dr.cost + slt_re.cost;

//     cand_slts.push_back(slt_re);

//     // send load table of parity merging
//     if (_code.isValidForPM() == true)
//     {
//         LoadTable slt_pm = getPartialSLTWIthParityMerging();
//         slt_pm.lt = Utils::dotAddUIntVectors(pslt_dr.lt, slt_pm.lt);
//         slt_pm.cost = pslt_dr.cost + slt_pm.cost;

//         cand_slts.push_back(slt_pm);
//     }

//     // for (auto &cand_slt : cand_slts)
//     // {
//     //     printf("stripe group: %ld, approach: %ld, cost: %ld, send load table:\n", _id, cand_slt.approach, cand_slt.cost);
//     //     Utils::printUIntVector(cand_slt.lt);
//     // }

//     return cand_slts;
// }

// LoadTable StripeGroup::getPartialSLTWithDataRelocation()
// {
//     size_t num_nodes = _settings.M;

//     LoadTable pslt_dr;
//     pslt_dr.lt = vector<size_t>(num_nodes, 0);

//     vector<size_t> data_distribution = getDataDistribution();

//     // it targets at general parameters (meaning that for a pre-transitioning group, there must exist no more than lambda_f data blocks in a node)
//     for (size_t node_id = 0; node_id < num_nodes; node_id++)
//     {
//         if (data_distribution[node_id] > _code.lambda_f)
//         {
//             size_t num_data_blocks_to_reloc = data_distribution[node_id] - _code.lambda_f;

//             pslt_dr.cost += num_data_blocks_to_reloc;

//             // update send load table
//             pslt_dr.lt[node_id] += num_data_blocks_to_reloc;
//         }
//     }

//     return pslt_dr;
// }

// LoadTable StripeGroup::getPartialSLTWithReEncoding()
// {
//     // relocation cost not considered in here

//     size_t num_nodes = _settings.M;
//     LoadTable pslt_re;
//     vector<size_t> data_distribution = getDataDistribution();

//     pslt_re.approach = EncodeMethod::RE_ENCODE;
//     pslt_re.lt = data_distribution; // all data blocks should be sent
//     for (size_t node_id = 0; node_id < num_nodes; node_id++)
//     {
//         pslt_re.cost += data_distribution[node_id];
//     }

//     return pslt_re;
// }

// LoadTable StripeGroup::getPartialSLTWIthParityMerging()
// {
//     LoadTable pslt_pm;
//     if (_code.isValidForPM() == false)
//     {
//         printf("invalid code for parity merging\n");
//         return pslt_pm;
//     }

//     size_t num_nodes = _settings.M;
//     vector<size_t> data_distribution = getDataDistribution();
//     vector<vector<size_t>> parity_distributions = getParityDistributions();

//     // relocation cost not considered in here
//     pslt_pm.lt = vector<size_t>(num_nodes, 0);
//     pslt_pm.approach = EncodeMethod::PARITY_MERGE;

//     for (size_t parity_id = 0; parity_id < _code.m_f; parity_id++)
//     {
//         // candidate nodes for parity merging
//         vector<size_t> &parity_distribution = parity_distributions[parity_id];

//         // check if there is a node with lambda parity blocks, if yes: then no need to do merging for the corresponding parity block
//         bool is_parity_aligned = false;
//         for (size_t node_id = 0; node_id < num_nodes; node_id++)
//         {

//             if (parity_distribution[node_id] == _code.lambda_i)
//             {
//                 is_parity_aligned = true;
//             }
//         }

//         // if not aligned, need to do merging
//         if (is_parity_aligned == false)
//         {
//             for (size_t node_id = 0; node_id < num_nodes; node_id++)
//             {

//                 if (parity_distribution[node_id] >= 1)
//                 {
//                     pslt_pm.lt[node_id] += 1;
//                     pslt_pm.cost += 1;
//                 }
//             }
//         }
//     }

//     return pslt_pm;
// }
