#include "StripeMerge.hh"

StripeMerge::StripeMerge(mt19937 &_random_generator) : random_generator(_random_generator)
{
}

StripeMerge::~StripeMerge()
{
}

void StripeMerge::genSolution(StripeBatch &stripe_batch)
{
    // check whether the code is valid for SM
    if (stripe_batch.code.isValidForPM() == false)
    {
        printf("invalid parameters for StripeMerge\n");
        return;
    }

    // Step 1: enumerate all possible stripe groups; pick non-overlapped stripe groups in ascending order of transition costs (bandwidth)
    printf("Step 1: construct stripe groups\n");
    stripe_batch.constructSGByCost();
    // stripe_batch.constructSGInSequence();
    // stripe_batch.constructSGByRandomPick();
    stripe_batch.print();

    // Step 2: generate transition solutions from all stripe groups
    printf("Step 2: generate transition solution\n");
    for (auto &item : stripe_batch.selected_sgs)
    {
        genSolution(item.second);
    }
}

void StripeMerge::genSolution(StripeGroup &stripe_group)
{
    ConvertibleCode &code = stripe_group.code;
    uint16_t num_nodes = stripe_group.settings.num_nodes;

    // record block distribution in final stripe (initialize with data blocks first)
    u16string final_block_dist = stripe_group.data_dist;

    // record current block placements in final stripe
    u16string cur_block_placement(code.n_f, INVALID_NODE_ID);

    // record current data block placement
    uint8_t final_block_id = 0;
    for (uint8_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
    {
        for (uint8_t block_id = 0; block_id < code.k_i; block_id++)
        {
            cur_block_placement[final_block_id] = stripe_group.pre_stripes[stripe_id]->indices[block_id];
            final_block_id++;
        }
    }

    // init final block placement (init with data blocks only)
    u16string final_block_placement(code.n_f, INVALID_NODE_ID);
    final_block_placement = cur_block_placement;

    /**
     * @brief Step 2.1: parity generation
     * for each parity block, find the node with minimum bandwidth for parity generation
     */

    // record minimum bw and corresponding placement
    uint8_t min_bw = UINT8_MAX;
    u16string min_bw_pm_nodes(code.m_f, INVALID_NODE_ID);

    // for parity merging, there are <num_nodes ^ code.m_f> possible choices to compute parity blocks, as we can collect each of m_f parity blocks at num_nodes nodes
    uint32_t num_pm_choices = pow(num_nodes, code.m_f);

    // enumerate m_f nodes for parity merging
    u16string pm_nodes(code.m_f, 0); // computation for parity i is at pm_nodes[i]

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
            min_bw_pm_nodes = pm_nodes;
        }

        // get next permutation
        Utils::getNextPerm(num_nodes, code.m_f, pm_nodes);
    }

    // update parity computation nodes, final block placement and block distribution
    stripe_group.parity_comp_nodes = min_bw_pm_nodes;
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        uint16_t cur_placed_node_id = min_bw_pm_nodes[parity_id];
        cur_block_placement[code.k_f + parity_id] = cur_placed_node_id;
        final_block_placement[code.k_f + parity_id] = cur_placed_node_id;
        final_block_dist[cur_placed_node_id]++;
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
        uint16_t cur_placed_node_id = cur_block_placement[final_block_id];
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
    stripe_group.parity_comp_method = EncodeMethod::PARITY_MERGE;
    stripe_group.parity_comp_nodes = min_bw_pm_nodes;
    stripe_group.post_stripe->indices = final_block_placement;
}