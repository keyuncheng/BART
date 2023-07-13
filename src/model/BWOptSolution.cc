#include "BWOptSolution.hh"

BWOptSolution::BWOptSolution(mt19937 &_random_generator) : random_generator(_random_generator)
{
}

BWOptSolution::~BWOptSolution()
{
}

void BWOptSolution::genSolution(StripeBatch &stripe_batch, string approach)
{
    // Step 1: enumerate a sufficiently large number of possible stripe
    // groups; pick non-overlapped stripe groups in ascending order of
    // transitioning bandwidth
    printf("Step 1: construct stripe groups\n");
    stripe_batch.constructSGByBWPartial(approach);
    // stripe_batch.constructSGByBWBF(approach);
    // stripe_batch.print();

    // Step 2: generate transition solutions from all stripe groups
    printf("Step 2: generate transition solution\n");
    for (auto &item : stripe_batch.selected_sgs)
    {
        genSolution(item.second, approach);
    }
}

void BWOptSolution::genSolution(StripeGroup &stripe_group, string approach)
{
    ConvertibleCode &code = stripe_group.code;
    uint16_t num_nodes = stripe_group.settings.num_nodes;

    /**
     * @brief Step 2.1: parity block generation
     * for each parity block, find the node with minimum bandwidth for parity generation
     */

    // record minimum bw and corresponding placement
    u16string enc_nodes(code.m_f, INVALID_NODE_ID);

    if (approach == "BWRE")
    { // re-encoding only
        stripe_group.getREBW(enc_nodes);
    }
    else if (approach == "BWPM")
    { // parity merging only
        stripe_group.getPMBW(enc_nodes);
    }

    /**
     * @brief Step 2.2: block relocation
     * for each node that placed with more than one block, relocate the corresponding blocks to other nodes without blocks
     */

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

    // update parity computation nodes, final block placement and block distribution
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        final_block_placement[code.k_f + parity_id] = enc_nodes[parity_id];
    }

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
    stripe_group.parity_comp_nodes = enc_nodes;
    stripe_group.post_stripe->indices = final_block_placement;
}