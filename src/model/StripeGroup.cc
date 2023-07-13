#include "StripeGroup.hh"

StripeGroup::StripeGroup(uint32_t _id, ConvertibleCode &_code, ClusterSettings &_settings, vector<Stripe *> &_pre_stripes, Stripe *_post_stripe) : id(_id), code(_code), settings(_settings), pre_stripes(_pre_stripes), post_stripe(_post_stripe)
{
    initDataDist();
    initParityDists();
}

StripeGroup::~StripeGroup()
{
}

void StripeGroup::print()
{
    printf("Stripe group %u: ", id);
    // for (auto stripe : pre_stripes)
    // {
    //     printf("%u, ", stripe->id);
    // }

    printf("\n");
    for (auto stripe : pre_stripes)
    {
        stripe->print();
    }
    printf("\n");
}

void StripeGroup::initDataDist()
{
    // init data distribution
    data_dist.assign(settings.num_nodes, 0);

    for (auto &stripe : pre_stripes)
    {
        for (uint8_t block_id = 0; block_id < code.k_i; block_id++)
        {
            data_dist[stripe->indices[block_id]] += 1;
        }
    }
}

void StripeGroup::initParityDists()
{
    // init parity distribution
    parity_dists.assign(code.m_i, u16string(settings.num_nodes, 0));

    for (uint8_t parity_id = 0; parity_id < code.m_i; parity_id++)
    {
        for (auto &stripe : pre_stripes)
        {
            uint16_t parity_node_id = stripe->indices[code.k_i + parity_id];
            parity_dists[parity_id][parity_node_id] += 1;
        }
    }
}

uint8_t StripeGroup::getTransBW(string approach, u16string &enc_nodes)
{
    uint8_t parity_update_bw = 0;

    if (approach == "BWRE" || approach == "BTRE")
    { // re-encoding only
        parity_update_bw = getREBW(enc_nodes);
    }
    else if (approach == "BWPM" || approach == "BTPM")
    { // parity-merging only
        parity_update_bw = getPMBW(enc_nodes);
    }
    else if (approach == "BT")
    { // mixed with re-encoding and parity merging
        // NOTE: here we assume that bandwidth(pm) <= bandwidth(re), thus we calculate pm bandwidth only
        parity_update_bw = getPMBW(enc_nodes);
    }
    else
    {
        fprintf(stderr, "invalid approach: %s\n", approach.c_str());
        exit(EXIT_FAILURE);
    }

    // data relocation bw
    uint8_t data_reloc_bw = getDataRelocBW();

    return parity_update_bw + data_reloc_bw;
}

uint8_t StripeGroup::getDataRelocBW()
{
    uint8_t data_reloc_bw = 0;

    // mark if each node is placed with a data block
    vector<bool> is_node_placed(settings.num_nodes, false);

    for (auto &stripe : pre_stripes)
    {
        for (uint8_t block_id = 0; block_id < code.k_i; block_id++)
        {
            uint16_t src_data_node = stripe->indices[block_id];
            if (is_node_placed[src_data_node] == false)
            {
                is_node_placed[src_data_node] = true;
            }
            else
            { // if the node is already placed with a data block
                data_reloc_bw++;
            }
        }
    }

    return data_reloc_bw;
}

uint8_t StripeGroup::getREBW(u16string &enc_nodes)
{
    // for re-encoding, find the node stored with most number of data blocks (at most code.lambda_i)
    uint16_t re_node = distance(data_dist.begin(), max_element(data_dist.begin(), data_dist.end()));
    uint8_t re_bw = code.k_f - data_dist[re_node];
    enc_nodes.assign(code.m_f, re_node);

    return re_bw;
}

uint8_t StripeGroup::getPMBW(u16string &enc_nodes)
{
    uint8_t num_pre_stripes = pre_stripes.size();

    uint8_t sum_pm_bw = 0;
    u16string placed_nodes = data_dist; // mark if nodes are placed with a block in the final stripe

    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        uint16_t min_bw_node = UINT16_MAX;
        uint8_t min_bw = UINT8_MAX;

        // we check <num_pre_stripe> nodes that stores the parity blocks, and
        // record where there are stored
        map<uint16_t, uint8_t> src_parity_nodes;
        for (uint8_t pre_stripe_id = 0; pre_stripe_id < num_pre_stripes; pre_stripe_id++)
        {
            uint16_t cand_pm_node = pre_stripes[pre_stripe_id]->indices[code.k_i + parity_id];

            if (src_parity_nodes.find(cand_pm_node) == src_parity_nodes.end())
            {
                src_parity_nodes.insert(pair<uint16_t, uint8_t>(cand_pm_node, 0));
            }

            src_parity_nodes[cand_pm_node]++;
        }

        // from the nodes that we recorded (at most <num_pre_stripe> nodes),
        // we find the node that creates the smallest bandwidth
        for (auto &item : src_parity_nodes)
        {
            uint16_t cand_pm_node = item.first;
            uint8_t num_src_parity_blocks = item.second;

            // bandwidth = the number of source parity blocks required +
            // new parity block relocation bandwidth
            uint8_t pm_bw = (num_pre_stripes - num_src_parity_blocks) + (placed_nodes[cand_pm_node] > 0 ? 1 : 0);

            if (pm_bw < min_bw)
            {
                min_bw = pm_bw;
                min_bw_node = cand_pm_node;

                // no need to search if min_bw = 0
                if (min_bw == 0)
                {
                    break;
                }
            }
        }

        // assign encode node
        enc_nodes[parity_id] = min_bw_node;
        placed_nodes[min_bw_node] += 1; // mark the node as placed with the new parity block
        sum_pm_bw += min_bw;            // update bw
    }

    return sum_pm_bw;
}

bool StripeGroup::isPerfectParityMerging()
{
    uint8_t num_required_parity_blocks = pre_stripes.size();

    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        // candidate nodes for parity merging
        u16string &parity_dist = parity_dists[parity_id];
        if (*max_element(parity_dist.begin(), parity_dist.end()) != num_required_parity_blocks)
        { // requires that there are num_required_parity_blocks parity blocks placed at a node
            return false;
        }
    }

    return true;
}

void StripeGroup::genParityComputeScheme4PerfectPM()
{
    // for perfect parity merging, parity generation generates zero transition bandwidth for
    applied_lt.approach = EncodeMethod::PARITY_MERGE;

    // find corresponding code.m_f nodes for perfect parity merging
    applied_lt.enc_nodes.assign(code.m_f, INVALID_NODE_ID);
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        u16string &parity_dist = parity_dists[parity_id];
        uint16_t min_bw_node_id = distance(parity_dist.begin(), max_element(parity_dist.begin(), parity_dist.end()));
        applied_lt.enc_nodes[parity_id] = min_bw_node_id;
    }

    // assign load tables
    applied_lt.slt.assign(settings.num_nodes, 0);
    applied_lt.rlt.assign(settings.num_nodes, 0);

    // bandwidth
    applied_lt.bw = 0;
}

void StripeGroup::genPartialLTs4ParityCompute(string approach)
{
    uint16_t num_nodes = settings.num_nodes;

    // for re-encoding, there are <num_nodes> possible candidate load tables, as we can collect num_final_data_blocks data blocks and distribute code.m_f parity blocks at <num_nodes> possible nodes
    uint32_t num_re_lts = num_nodes;
    // for parity merging, there are <num_nodes ^ code.m_f> possible candidate load tables, as we can collect each of m_f parity blocks at num_nodes nodes
    uint32_t num_pm_lts = pow(num_nodes, code.m_f);

    // enumerate partial load tables for re-encoding
    vector<LoadTable> cand_re_lts(num_re_lts);
    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        cand_re_lts[node_id] = genPartialLT4ParityCompute(EncodeMethod::RE_ENCODE, u16string(code.m_f, node_id));
    }

    // enumerate partial load tables for parity merging
    vector<LoadTable> cand_pm_lts(num_pm_lts);
    u16string pm_nodes(code.m_f, 0); // computation for parity i is at pm_nodes[i]
    for (uint32_t perm_id = 0; perm_id < num_pm_lts; perm_id++)
    {
        cand_pm_lts[perm_id] = genPartialLT4ParityCompute(EncodeMethod::PARITY_MERGE, pm_nodes);

        // get next permutation
        Utils::getNextPerm(num_nodes, code.m_f, pm_nodes);
    }

    // create load tables
    cand_partial_lts.clear();
    if (approach == "BTRE")
    { // use re-encoding only
        cand_partial_lts.insert(cand_partial_lts.end(), make_move_iterator(cand_re_lts.begin()), make_move_iterator(cand_re_lts.end()));
    }
    else if (approach == "BTPM")
    { // use parity merging only
        cand_partial_lts.insert(cand_partial_lts.end(), make_move_iterator(cand_pm_lts.begin()), make_move_iterator(cand_pm_lts.end()));
    }
    else if (approach == "BT")
    { // use both
        cand_partial_lts.insert(cand_partial_lts.end(), make_move_iterator(cand_re_lts.begin()), make_move_iterator(cand_re_lts.end()));
        cand_partial_lts.insert(cand_partial_lts.end(), make_move_iterator(cand_pm_lts.begin()), make_move_iterator(cand_pm_lts.end()));
    }
}

LoadTable StripeGroup::genPartialLT4ParityCompute(EncodeMethod enc_method, u16string enc_nodes)
{
    uint16_t num_nodes = settings.num_nodes;

    uint8_t num_pre_stripes = pre_stripes.size();
    uint8_t num_final_data_blocks = num_pre_stripes * code.k_i;
    uint8_t num_required_parity_blocks = num_pre_stripes;

    LoadTable lt;
    lt.approach = enc_method;
    lt.enc_nodes = enc_nodes;
    if (enc_method == EncodeMethod::RE_ENCODE)
    {
        uint16_t enc_node = enc_nodes[0];                               // encode node
        lt.slt = data_dist;                                             // send load table (send data blocks)
        lt.slt[enc_node] = code.m_f;                                    // only need to send the **parity blocks** at <node_id>
        lt.rlt.assign(num_nodes, 0);                                    // recv load table
        lt.rlt[enc_node] = num_final_data_blocks - data_dist[enc_node]; // recv data blocks at <node_id>
        lt.bw = num_final_data_blocks - data_dist[enc_node] + code.m_f;
    }
    else if (enc_method == EncodeMethod::PARITY_MERGE)
    {
        lt.slt.assign(num_nodes, 0); // send load table
        lt.rlt.assign(num_nodes, 0); // recv load table

        // obtain temp block placement (with data blocks and newly computed parity blocks)
        u16string temp_block_placement = data_dist;
        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            temp_block_placement[enc_nodes[parity_id]]++;
        }

        // obtain the load distribution for parity blocks
        for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
        {
            uint16_t parity_comp_node_id = enc_nodes[parity_id];
            // send load dist

            // collect parity block
            for (uint8_t stripe_id = 0; stripe_id < num_pre_stripes; stripe_id++)
            {
                uint16_t parity_node_id = pre_stripes[stripe_id]->indices[code.k_i + parity_id];
                if (parity_node_id != parity_comp_node_id)
                {
                    lt.slt[parity_node_id]++;
                }
            }

            // compute at parity_comp_node_id; if there is a data block located there, then we need to relocate the parity block
            if (temp_block_placement[parity_comp_node_id] > 1)
            {
                lt.slt[parity_comp_node_id]++;
                temp_block_placement[parity_comp_node_id]--; // mark the block has been transferred out
            }

            lt.rlt[parity_comp_node_id] += num_required_parity_blocks - parity_dists[parity_id][parity_comp_node_id]; // number of required parity block for parity generation
        }

        lt.bw = accumulate(lt.slt.begin(), lt.slt.end(), 0); // update bandwidth (for send load)
    }

    return lt;
}