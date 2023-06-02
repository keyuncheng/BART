#include "StripeMerge.hh"

StripeMerge::StripeMerge(mt19937 &_random_generator) : random_generator(_random_generator)
{
}

StripeMerge::~StripeMerge()
{
}

void StripeMerge::genSolution(StripeBatch &stripe_batch, string approach)
{
    // Step 1: enumerate all possible stripe groups; pick non-overlapped stripe groups in ascending order of transition costs (bandwidth)
    printf("Step 1: construct stripe groups\n");
    // stripe_batch.constructSGByBW(approach);
    stripe_batch.constructSGByBWGreedy(approach);
    // stripe_batch.print();

    // Step 2: generate transition solutions from all stripe groups
    printf("Step 2: generate transition solution\n");
    for (auto &item : stripe_batch.selected_sgs)
    {
        genSolution(item.second, approach);
    }
}

void StripeMerge::genSolution(StripeGroup &stripe_group, string approach)
{
    ConvertibleCode &code = stripe_group.code;
    uint16_t num_nodes = stripe_group.settings.num_nodes;

    // record final block placements in final stripe
    u16string final_block_placement(code.n_f, INVALID_NODE_ID);

    // record current data block placement
    for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
    {
        for (uint8_t block_id = 0; block_id < code.k_i; block_id++)
        {
            uint8_t final_block_id = stripe_id * code.k_i + block_id;
            final_block_placement[final_block_id] = stripe_group.pre_stripes[stripe_id]->indices[block_id];
        }
    }

    /**
     * @brief Step 2.1: parity generation
     * for each parity block, find the node with minimum bandwidth for parity generation
     */

    // record minimum bw and corresponding placement
    uint8_t min_bw = UINT8_MAX;
    u16string min_bw_pm_nodes(code.m_f, INVALID_NODE_ID);

    if (approach == "BWRE")
    { // re-encoding only
        // for re-encoding, find the node with most number of data blocks (at most code.lambda_i)
        uint8_t max_num_placed_data_blocks = *max_element(stripe_group.data_dist.begin(), stripe_group.data_dist.end());

        // randomly find a node with max_num_placed_data_blocks
        vector<uint16_t> valid_parity_compute_nodes;
        for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
        {
            if (stripe_group.data_dist[node_id] == max_num_placed_data_blocks)
            {
                valid_parity_compute_nodes.push_back(node_id);
            }
        }

        // randomly find a solution
        size_t rand_pos = Utils::randomUInt(0, valid_parity_compute_nodes.size() - 1, random_generator);
        uint16_t parity_compute_node = valid_parity_compute_nodes[rand_pos];
        min_bw_pm_nodes.assign(code.m_f, parity_compute_node);
    }
    else if (approach == "BWPM")
    { // parity merging only
        // for parity merging, there are <num_nodes ^ code.m_f> possible choices to compute parity blocks, as we can collect each of m_f parity blocks at num_nodes nodes
        uint32_t num_pm_choices = pow(num_nodes, code.m_f);

        // enumerate m_f nodes for parity merging
        u16string pm_nodes(code.m_f, 0); // computation for parity i is at pm_nodes[i]

        vector<u16string> valid_pm_nodes;

        for (uint32_t perm_id = 0; perm_id < num_pm_choices; perm_id++)
        {
            u16string relocated_nodes = stripe_group.data_dist; // mark the number of blocks relocated on the node
            uint8_t cur_perm_bw = 0;                            // record current bw for perm_id
            for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
            {
                u16string &parity_dist = stripe_group.parity_dists[parity_id];
                uint16_t parity_compute_node = pm_nodes[parity_id];

                uint8_t pm_bw = (code.alpha - parity_dist[parity_compute_node]) + (relocated_nodes[parity_compute_node] > 0 ? 1 : 0); // required number of parity blocks + parity relocation bw

                relocated_nodes[parity_compute_node] += 1; // mark the node as relocated on the node
                cur_perm_bw += pm_bw;                      // update bw
            }

            // update minimum bw
            if (cur_perm_bw < min_bw)
            {
                min_bw = cur_perm_bw;

                // reset best solution
                valid_pm_nodes.clear();
                valid_pm_nodes.push_back(pm_nodes);
            }
            else if (cur_perm_bw == min_bw)
            {
                valid_pm_nodes.push_back(pm_nodes);
            }

            // get next permutation
            Utils::getNextPerm(num_nodes, code.m_f, pm_nodes);
        }

        // randomly find a valid solution
        size_t rand_pos = Utils::randomUInt(0, valid_pm_nodes.size() - 1, random_generator);
        min_bw_pm_nodes = valid_pm_nodes[rand_pos];
    }

    // update parity computation nodes, final block placement and block distribution
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        final_block_placement[code.k_f + parity_id] = min_bw_pm_nodes[parity_id];
    }

    /**
     * @brief Step 2.2: block relocation
     * for each node that placed with more than one block, relocate the corresponding blocks to other nodes without blocks
     */

    // find blocks that needs relocation
    vector<bool> is_node_placed(num_nodes, false);
    vector<uint8_t> blocks_to_reloc;
    for (uint8_t final_block_id = 0; final_block_id < code.n_f; final_block_id++)
    {
        uint16_t cur_placed_node_id = final_block_placement[final_block_id];
        if (is_node_placed[cur_placed_node_id] == false)
        {
            is_node_placed[cur_placed_node_id] = true;
        }
        else
        {
            blocks_to_reloc.push_back(final_block_id);
        }
    }

    vector<uint16_t> available_reloc_nodes; // available nodes for block relocation
    for (uint16_t node_id = 0; node_id < num_nodes; node_id++)
    {
        if (is_node_placed[node_id] == false)
        {
            // add node as candidate nodes for relocation
            available_reloc_nodes.push_back(node_id);
        }
    }

    // shuffle the available nodes and store in the first <blocks_to_reloc> nodes
    shuffle(available_reloc_nodes.begin(), available_reloc_nodes.end(), random_generator);

    // create block transfer tasks
    for (uint8_t idx = 0; idx < blocks_to_reloc.size(); idx++)
    {
        uint8_t final_block_id = blocks_to_reloc[idx];
        uint16_t dst_node = available_reloc_nodes[idx];

        // record final block placement
        final_block_placement[final_block_id] = dst_node;
    }

    // update stripe group metadata
    if (approach == "BWRE")
    {
        stripe_group.parity_comp_method = EncodeMethod::RE_ENCODE;
    }
    else if (approach == "BWPM")
    {
        stripe_group.parity_comp_method = EncodeMethod::PARITY_MERGE;
    }
    else
    {
        stripe_group.parity_comp_method = EncodeMethod::UNKNOWN_METHOD;
    }
    stripe_group.parity_comp_nodes = min_bw_pm_nodes;
    stripe_group.post_stripe->indices = final_block_placement;
}