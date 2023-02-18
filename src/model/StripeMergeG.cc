#include "StripeMergeG.hh"

StripeMergeG::StripeMergeG(/* args */)
{
    std::srand(std::time(0));
}

StripeMergeG::~StripeMergeG()
{
}

void StripeMergeG::getSolutionForStripeBatch(StripeBatch &stripe_batch, vector<vector<int> > &solutions, mt19937 random_generator) {
    ConvertibleCode &code = stripe_batch.getCode();
    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    // initialize the solution
    solutions.clear();

    // check whether the code is valid for SM
    if (code.isValidForPM() == false) {
        printf("invalid parameters for StripeMerge\n");
        return;
    }
    
    for (size_t sg_id = 0; sg_id < stripe_groups.size(); sg_id++) {
        StripeGroup &stripe_group = stripe_groups[sg_id];
        // get transition solution for each stripe group
        vector<vector<int> > sg_solutions;
        getSolutionForStripeGroup(stripe_group, sg_solutions, random_generator);

        // put it into the solution for the batch
        printf("solution for stripe_group %ld (size: %ld): \n", sg_id, sg_solutions.size());
        for (auto sg_solution : sg_solutions) {
            printf("stripe %d, block %d, from: %d, to: %d\n", sg_solution[0], sg_solution[1], sg_solution[2], sg_solution[3]);
            solutions.push_back(sg_solution);
        }
    }

    return;
}

void StripeMergeG::getSolutionForStripeGroup(StripeGroup &stripe_group, vector<vector<int> > &solutions, mt19937 random_generator) {
    ConvertibleCode &code = stripe_group.getCode();
    ClusterSettings &settings = stripe_group.getClusterSettings();

    int num_nodes = settings.M;

    vector<Stripe *> &stripes = stripe_group.getStripes();

    // get distributions
    vector<int> data_distribution = stripe_group.getDataDistribution();
    vector<vector<int> > parity_distributions = stripe_group.getParityDistributions();

    // printf("data distribution:\n");
    // Utils::printIntVector(data_distribution);

    // printf("parity distributions:\n");
    // for (int parity_id = 0; parity_id < code.m_i; parity_id++) {
    //     printf("parity block %d:\n", parity_id);
    //     Utils::printIntVector(parity_distributions[parity_id]);
    // }
    
    // TODO: Step 1: data relocation; Step 2: parity relocation

    // relocated stripe distribution
    vector<bool> relocated_blk_distribution(num_nodes, false);

    // Step 1: data relocation

    // candidate nodes for data relocation (no any data block stored on that node)
    vector<int> data_reloc_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        // find nodes store more than lambda_f data block (in parity merging, lambda_f = 1, we find nodes where no data block is stored, and no parity block is relocated)
        if (data_distribution[node_id] < code.lambda_f) {
            data_reloc_candidates.push_back(node_id);
        } else {
            // mark the block as relocated
            relocated_blk_distribution[node_id] = true;
        }
    }

    // printf("data relocation candidates:\n");
    // Utils::printIntVector(data_reloc_candidates);

    // record data blocks that needs relocation (stripe_id, block_id)
    vector<pair<int, int> > data_blocks_to_reloc;

    // relocate data blocks
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        // find nodes store more than 1 data block
        if (data_distribution[node_id] <= code.lambda_f) {
            continue;
        }
        int num_db_overlapped = 0;
        for (int stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
            vector<int> &stripe_indices = stripes[stripe_id]->getStripeIndices();
            for (int block_id = 0; block_id < code.k_i; block_id++) {
                // find block_id that stored at node_id
                if (stripe_indices[block_id] != node_id) {
                    continue;
                }
                num_db_overlapped++;
                // do relocation starting from the 2nd overlapped data block
                if (num_db_overlapped > code.lambda_f) {
                    data_blocks_to_reloc.push_back(pair<int, int>(stripe_id, block_id));
                }
            }
        }
    }

    // create data relocation task for each data block
    for (auto &item : data_blocks_to_reloc) {
        int stripe_id = item.first;
        int block_id = item.second;

        vector<int> &stripe_indices = stripes[stripe_id]->getStripeIndices();
        int node_id = stripe_indices[block_id];

        // Strategy 1: randomly pick a node from data relocation candidates
        int pos = Utils::randomInt(0, data_reloc_candidates.size() - 1, random_generator);

        // // Strategy 2: pick the first node from data relocation candidates
        // int pos = 0;

        // relocate node id
        int reloc_node_id = data_reloc_candidates[pos];

        // mark the block as relocated
        relocated_blk_distribution[reloc_node_id] = true;

        // remove the element from data relocation candidate
        data_reloc_candidates.erase(data_reloc_candidates.begin() + pos);
        // update the data distribution
        data_distribution[node_id]--;
        data_distribution[reloc_node_id]++;

        // create a data relocation task
        vector<int> solution;
        solution.push_back(stripe_id);
        solution.push_back(block_id);
        solution.push_back(node_id);
        solution.push_back(reloc_node_id);

        solutions.push_back(solution);
    }


    // Step 2: parity relocation

    // all nodes are candidates for parity block computation
    vector<int> parity_comp_candidates(num_nodes, 0);
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        parity_comp_candidates[node_id] = node_id;
    }

    /** 2.1 for each parity block, find the node with minimum merging cost to calculate new parity block
     * if the node with min cost already stores a block (a data block or a parity block), then we need to relocate the parity block to another node
     */
    for (int parity_id = 0; parity_id < code.m_f; parity_id++) {
        // get parity distribution
        vector<int> &parity_distribution = parity_distributions[parity_id];

        // compute costs for the parity block at each node 
        vector<int> pm_costs(num_nodes, 0);

        for (int node_id = 0; node_id < num_nodes; node_id++) {
            pm_costs[node_id] = code.alpha - parity_distribution[node_id];
            // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
            if (relocated_blk_distribution[node_id] == true) {
                pm_costs[node_id] += 1;
            }
        }

        // get the one minimum merging cost
        int min_pm_cost = *min_element(pm_costs.begin(), pm_costs.end());

        // find all nodes with minimum merging cost
        vector<int> min_cost_nodes;
        for (int node_id = 0; node_id < num_nodes; node_id++) {
            if (pm_costs[node_id] == min_pm_cost) {
                min_cost_nodes.push_back(node_id);
            }
        }

        // randomly pick one node with minimum cost to compute
        int random_pos = Utils::randomInt(0, min_cost_nodes.size() - 1, random_generator);
        int parity_comp_node_id = min_cost_nodes[random_pos];

        // create a parity relocation task (collect parity blocks for new parity computation)
        for (int stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
            vector<int> &stripe_indices = stripes[stripe_id]->getStripeIndices();

            // add a task if the parity block is not stored at the relocated node
            if (stripe_indices[code.k_i + parity_id] != parity_comp_node_id) {
                vector<int> solution;
                solution.push_back(stripe_id); // stripe_id
                solution.push_back(code.k_i + parity_id); // block_id
                solution.push_back(stripe_indices[code.k_i + parity_id]); // from node_id
                solution.push_back(parity_comp_node_id); // to node_id
                
                solutions.push_back(solution);
            }
        }

        // check if parity_comp_node_id already stores a block
        if (relocated_blk_distribution[parity_comp_node_id] == false) {
            // if the node doesn't store a block, then we don't need to relocate
            // mark as relocated
            relocated_blk_distribution[parity_comp_node_id] = true;
        } else {
            // if the node stores a parity block, then we need to relocate to another node

            // candidate nodes for parity block relocation (nodes without a block)
            vector<int> parity_reloc_candidates;
            for (int node_id = 0; node_id < num_nodes; node_id++) {
                if (relocated_blk_distribution[node_id] == false) {
                    parity_reloc_candidates.push_back(node_id);
                }
            }

            // randomly pick one node with minimum cost to compute
            int random_pos = Utils::randomInt(0, parity_reloc_candidates.size() - 1, random_generator);
            int parity_reloc_node_id = parity_reloc_candidates[random_pos];

            // create a parity relocation task
            vector<int> solution;
            solution.push_back(-1); // stripe_id as -1
            solution.push_back(code.k_f + parity_id); // new block_id (k_f + parity_id)
            solution.push_back(parity_comp_node_id); // from node_id
            solution.push_back(parity_reloc_node_id); // to node_id
            
            solutions.push_back(solution);
        }
        
    }

    // printf("parity_relocated_nodes:\n");
    // Utils::printIntVector(parity_relocated_nodes);



    // // now the remaining data relocation nodes will be used for parity merging
    // printf("data relocation candidates (after data relocation):\n");
    // Utils::printIntVector(data_reloc_candidates);
    
}