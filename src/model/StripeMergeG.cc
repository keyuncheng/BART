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
        printf("invalid parameters\n");
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

    // check whether the code is valid for SM
    if (code.isValidForPM() == false) {
        printf("invalid parameters\n");
        return;
    }

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
    
    // Step 1: parity relocation

    vector<int> parity_relocated_nodes; // nodes that are already relocated with a parity block

    // find the data blocks that needs relocation
    for (int parity_id = 0; parity_id < code.m_f; parity_id++) {
        // get parity distribution
        vector<int> &parity_distribution = parity_distributions[parity_id];

        // candidate nodes for parity relocation (all nodes except those already relocated with a parity block)
        vector<int> parity_reloc_candidates;
        for (int node_id = 0; node_id < num_nodes; node_id++) {
            if (find(parity_relocated_nodes.begin(), parity_relocated_nodes.end(), node_id) == parity_relocated_nodes.end()) {
                parity_reloc_candidates.push_back(node_id);
            }
        }

        // calculate merge cost for each candidate
        int num_preloc_candidates = parity_reloc_candidates.size();
        vector<int> pm_costs(num_preloc_candidates, 0);

        for (int idx = 0; idx < num_preloc_candidates; idx++) {
            int node_id = parity_reloc_candidates[idx];
            pm_costs[idx] = code.alpha - parity_distribution[node_id];
            // the node already stores a data block, need to relocate to another node for merging
            if (data_distribution[node_id] > 0) {
                pm_costs[idx] += 1;
            }
        }

        // get the one minimum merging cost
        int min_pm_cost = *min_element(pm_costs.begin(), pm_costs.end());

        // find all nodes with minimum merging cost
        vector<int> min_cost_nodes;
        for (int idx = 0; idx < num_preloc_candidates; idx++) {
            if (pm_costs[idx] == min_pm_cost) {
                int node_id = parity_reloc_candidates[idx];
                min_cost_nodes.push_back(node_id);
            }
        }

        // randomly pick one node with minimum cost to relocate
        int random_pos = Utils::randomInt(0, min_cost_nodes.size() - 1, random_generator);
        int parity_reloc_node_id = min_cost_nodes[random_pos];

        // mark the node as relocated with parity_id
        parity_relocated_nodes.push_back(parity_reloc_node_id);

        // create a data relocation task
        for (int stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
            vector<int> &stripe_indices = stripes[stripe_id]->getStripeIndices();

            // add a task if the parity block is not stored at the relocated node
            if (stripe_indices[code.k_i + parity_id] != parity_reloc_node_id) {
                vector<int> solution;
                solution.push_back(stripe_id); // stripe_id
                solution.push_back(code.k_i + parity_id); // block_id
                solution.push_back(stripe_indices[code.k_i + parity_id]); // from node_id
                solution.push_back(parity_reloc_node_id); // to node_id
                
                solutions.push_back(solution);
            }
        }
    }

    // printf("parity_relocated_nodes:\n");
    // Utils::printIntVector(parity_relocated_nodes);

    // Step 2: data relocation

    // candidate nodes for data relocation (no any data block stored on that node)
    vector<int> data_reloc_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        // find nodes store more than lambda_f data block (in parity merging, lambda_f = 1, we find nodes where no data block is stored, and no parity block is relocated)
        if (data_distribution[node_id] < code.lambda_f && (find(parity_relocated_nodes.begin(), parity_relocated_nodes.end(), node_id) == parity_relocated_nodes.end())) {
            data_reloc_candidates.push_back(node_id);
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

    // if the node (will be relocated with a parity block) already stores a data block, need to relocate to other nodes
    for (auto node_id : parity_relocated_nodes) {
        if (data_distribution[node_id] == 0) {
            continue;
        }

        bool overlapped_dblk_found = false;
        // find the first occurence at the node
        for (int stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
            vector<int> &stripe_indices = stripes[stripe_id]->getStripeIndices();
            for (int block_id = 0; block_id < code.k_i; block_id++) {
                // find data block_id that stored at node_id
                if (stripe_indices[block_id] == node_id) {
                    data_blocks_to_reloc.push_back(pair<int, int>(stripe_id, block_id));
                    overlapped_dblk_found = true;
                    break;
                }
            }
            
            if (overlapped_dblk_found == true) {
                break;
            } 
        }
    }

    // create data relocation task for each data block
    for (auto &item : data_blocks_to_reloc) {
        int stripe_id = item.first;
        int block_id = item.second;

        vector<int> &stripe_indices = stripes[stripe_id]->getStripeIndices();
        int node_id = stripe_indices[block_id];

        // randomly pick a node from data relocation candidates
        int random_pos = Utils::randomInt(0, data_reloc_candidates.size() - 1, random_generator);
        int reloc_node_id = data_reloc_candidates[random_pos];

        // remove the element from data relocation candidate
        data_reloc_candidates.erase(data_reloc_candidates.begin() + random_pos);
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

    // // now the remaining data relocation nodes will be used for parity merging
    // printf("data relocation candidates (after data relocation):\n");
    // Utils::printIntVector(data_reloc_candidates);
    
}