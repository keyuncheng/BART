#include "StripeMergeG.hh"

StripeMergeG::StripeMergeG(/* args */)
{
    std::srand(std::time(0));
}

StripeMergeG::~StripeMergeG()
{
}

void StripeMergeG::getSolutionForStripeBatch(StripeBatch &stripe_batch, vector<vector<int> > &solutions) {
    ConvertibleCode &code = stripe_batch.getCode();
    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    // if k' = alpha * k
    if (code.k_f != code.k_i * code.alpha || code.m_f > code.m_i) {
        printf("invalid parameters\n");
        return;
    }
    
    for (auto &stripe_group : stripe_groups) {
        vector<vector<int> > sg_solutions;
        getSolutionForStripeGroup(stripe_group, sg_solutions);

        for (auto sg_solution : sg_solutions) {
            solutions.push_back(sg_solution);
        }
    }

    return;
}

void StripeMergeG::getSolutionForStripeGroup(StripeGroup &stripe_group, vector<vector<int> > &solutions) {
    ConvertibleCode &code = stripe_group.getCode();
    ClusterSettings &settings = stripe_group.getClusterSettings();

    // if k' = alpha * k
    if (code.k_f != code.k_i * code.alpha || code.m_f > code.m_i) {
        printf("invalid parameters\n");
        return;
    }

    int num_nodes = settings.M;

    vector<Stripe> &stripes = stripe_group.getStripes();
    vector<int> num_data_stored(num_nodes, 0);

     // check which node already stores at least one data block
    for (auto stripe : stripes) {
        vector<int> &stripe_indices = stripe.getStripeIndices();

        for (int block_id = 0; block_id < code.k_i; block_id++) {
            num_data_stored[stripe_indices[block_id]] += 1;
        }
    }

    printf("data block distribution:\n");
    Utils::printIntVector(num_data_stored);

    // candidate nodes for data relocation (no any data block stored on that node)
    vector<int> data_relocation_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] == 0) {
            data_relocation_candidates.push_back(node_id);
        }
    }

    // printf("data relocation candidates:\n");
    // Utils::printIntVector(data_relocation_candidates);

    // relocate data blocks
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] > 1) {
            bool db_overlapped = false;
            for (int stripe_id = 0; stripe_id < code.alpha; stripe_id++) {
                vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();
                for (int block_id = 0; block_id < code.k_i; block_id++) {
                    if (stripe_indices[block_id] == node_id) {
                        if (db_overlapped == false) {
                            db_overlapped = true;
                            break;
                        } else {
                            // randomly pick a node from data relocation candidates
                            int random_pos = std::rand() % data_relocation_candidates.size();
                            int reloc_node_id = data_relocation_candidates[random_pos];

                            printf("%d\n", reloc_node_id);

                            // remove the element from data relocation candidate
                            data_relocation_candidates.erase(data_relocation_candidates.begin() + random_pos);

                            // create a data relocation task
                            vector<int> solution;
                            solution.push_back(stripe_id);
                            solution.push_back(block_id);
                            solution.push_back(node_id);
                            solution.push_back(reloc_node_id);

                            solutions.push_back(solution);
                        }
                    }
                }
            }
        }
    }

    // now the remaining data relocation nodes will be used for parity merging

    // parity relocation

    // candidate nodes for parity relocation (the same as data relocation)
    vector<int> parity_relocation_candidates = data_relocation_candidates;

    // printf("parity relocation candidates:\n");
    // Utils::printIntVector(parity_relocation_candidates);

    // relocate for every parity block
    for (int parity_id = 0; parity_id < code.m_f; parity_id++) {
        map<int, vector<int>> parity_dist;

        for (auto parity_relocation_candidate : parity_relocation_candidates) {
            // identify the stripe ids that parity_id is NOT stored at parity_relocation_candidate
            for (int stripe_id = 0; stripe_id < code.alpha; stripe_id++) {
                vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();
                if (stripe_indices[code.k_i + parity_id] != parity_relocation_candidate) {
                    parity_dist[parity_relocation_candidate].push_back(stripe_id);
                }
            }
        }

        // printf("map:\n");
        // for (auto it = parity_dist.begin(); it != parity_dist.end(); it++) {
        //     printf("%d: ", it->first);
        //     Utils::printIntVector(it->second);
        // }

        // find the minimum number of blocks required
        int min_num_blocks_for_merging = INT_MAX;
        for (auto it = parity_dist.begin(); it != parity_dist.end(); it++) {
            vector<int> &p_stripes = it->second;
            if ((int) p_stripes.size() < min_num_blocks_for_merging) {
                min_num_blocks_for_merging = (int) p_stripes.size();
            }
        }

        vector<int> final_node_candidates;
        for (auto it = parity_dist.begin(); it != parity_dist.end(); it++) {
            vector<int> &p_stripes = it->second;
            if ((int) p_stripes.size() == min_num_blocks_for_merging) {
                final_node_candidates.push_back(it->first);
            }
        }

        int random_pos = std::rand() % final_node_candidates.size();
        int parity_reloc_node_id = final_node_candidates[random_pos];

        // remove the element from data relocation candidate
        parity_relocation_candidates.erase(parity_relocation_candidates.begin() + random_pos);

        // create a data relocation task
        for (auto stripe_id : parity_dist[parity_reloc_node_id]) {
            vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();

            vector<int> solution;
            solution.push_back(stripe_id);
            solution.push_back(code.k_i + parity_id);
            solution.push_back(stripe_indices[code.k_i + parity_id]);
            solution.push_back(parity_reloc_node_id);
            
            solutions.push_back(solution);
        }
    }
}

void StripeMergeG::getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<int> > &solutions, vector<int> &load_dist) {
    int num_nodes = settings.M;

    load_dist.resize(num_nodes);
    for (auto &item : load_dist) {
        item = 0;
    }

    for (auto &solution : solutions) {
        // int from_node_id = solution[2];
        int to_node_id = solution[3];
        load_dist[to_node_id] += 1;
    }

    return;
}