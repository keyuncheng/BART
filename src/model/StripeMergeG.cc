// #include "StripeMergeG.hh"

// StripeMergeG::StripeMergeG(/* args */)
// {
//     std::srand(std::time(0));
// }

// StripeMergeG::~StripeMergeG()
// {
// }

// void StripeMergeG::getSolutionForStripeBatch(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator)
// {
//     ConvertibleCode &code = stripe_batch.getCode();
//     vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

//     // initialize the solution
//     solutions.clear();

//     // check whether the code is valid for SM
//     if (code.isValidForPM() == false)
//     {
//         printf("invalid parameters for StripeMerge\n");
//         return;
//     }

//     for (size_t sg_id = 0; sg_id < stripe_groups.size(); sg_id++)
//     {
//         StripeGroup &stripe_group = stripe_groups[sg_id];
//         // get transition solution for each stripe group
//         vector<vector<size_t>> sg_solutions;
//         getSolutionForStripeGroup(stripe_group, sg_solutions, random_generator);

//         // put it into the solution for the batch
//         printf("solution for stripe_group %ld (size: %ld): \n", sg_id, sg_solutions.size());
//         for (auto sg_solution : sg_solutions)
//         {
//             printf("stripe %ld, block %ld, from: %ld, to: %ld\n", sg_solution[0], sg_solution[1], sg_solution[2], sg_solution[3]);
//             solutions.push_back(sg_solution);
//         }
//     }

//     return;
// }

// void StripeMergeG::getSolutionForStripeGroup(StripeGroup &stripe_group, vector<vector<size_t>> &solutions, mt19937 random_generator)
// {
//     ConvertibleCode &code = stripe_group.getCode();
//     ClusterSettings &settings = stripe_group.getClusterSettings();

//     size_t num_nodes = settings.M;

//     vector<Stripe *> &stripes = stripe_group.getStripes();

//     // get distributions
//     vector<size_t> data_distribution = stripe_group.getDataDistribution();
//     vector<vector<size_t>> parity_distributions = stripe_group.getParityDistributions();

//     // printf("data distribution:\n");
//     // Utils::printUIntVector(data_distribution);

//     // printf("parity distributions:\n");
//     // for (size_t parity_id = 0; parity_id < code.m_i; parity_id++) {
//     //     printf("parity block %d:\n", parity_id);
//     //     Utils::printUIntVector(parity_distributions[parity_id]);
//     // }

//     // relocated block distribution
//     vector<bool> relocated_blk_distribution(num_nodes, false);

//     // candidate nodes for block relocation
//     vector<size_t> reloc_node_candidates;

//     for (size_t node_id = 0; node_id < num_nodes; node_id++)
//     {
//         // mark nodes with at least one data block as relocated
//         if (data_distribution[node_id] > 0)
//         {
//             relocated_blk_distribution[node_id] = true;
//         }
//         else
//         {
//             reloc_node_candidates.push_back(node_id);
//         }
//     }

//     // printf("node candidates for block relocation (before PM):\n");
//     // Utils::printUIntVector(reloc_node_candidates);

//     // Step 1: parity computation and relocation

//     // parity block computation node candidates: all nodes are candidates
//     vector<size_t> parity_comp_candidates(num_nodes, 0);
//     for (size_t node_id = 0; node_id < num_nodes; node_id++)
//     {
//         parity_comp_candidates[node_id] = node_id;
//     }

//     /** for each parity block, find the node with minimum merging cost to calculate new parity block
//      * if the node with min cost already stores a block (data or parity), then we need to relocate the newly computed parity block to another node
//      */
//     for (size_t parity_id = 0; parity_id < code.m_f; parity_id++)
//     {
//         // get parity distribution
//         vector<size_t> &parity_distribution = parity_distributions[parity_id];

//         // compute costs for the parity block at each node
//         vector<int> pm_costs(num_nodes, 0);

//         for (size_t node_id = 0; node_id < num_nodes; node_id++)
//         {
//             pm_costs[node_id] = code.alpha - parity_distribution[node_id];
//             // the node already stores a data block, need to relocate either the data block or the computed parity block to another node
//             if (relocated_blk_distribution[node_id] == true)
//             {
//                 pm_costs[node_id] += 1;
//             }
//         }

//         // find minimum cost node among all nodes
//         size_t min_cost_node = distance(pm_costs.begin(), min_element(pm_costs.begin(), pm_costs.end()));

//         size_t parity_comp_node_id = min_cost_node;

//         // create a parity relocation task (collect parity blocks for new parity computation)
//         for (size_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
//         {
//             vector<size_t> &stripe_indices = stripes[stripe_id]->getStripeIndices();

//             // add a task for parity blocks not stored at the node
//             if (stripe_indices[code.k_i + parity_id] != parity_comp_node_id)
//             {
//                 vector<size_t> solution;
//                 solution.push_back(stripe_id);                            // stripe_id
//                 solution.push_back(code.k_i + parity_id);                 // block_id
//                 solution.push_back(stripe_indices[code.k_i + parity_id]); // from node_id
//                 solution.push_back(parity_comp_node_id);                  // to node_id

//                 solutions.push_back(solution);
//             }
//         }

//         // check if parity_comp_node_id already stores a block
//         if (relocated_blk_distribution[parity_comp_node_id] == false)
//         {
//             // if the node doesn't store a block, then we don't need to relocate to other nodes

//             // mark the node as relocated
//             relocated_blk_distribution[parity_comp_node_id] = true;

//             // remove from candidate node list
//             auto it = find(reloc_node_candidates.begin(), reloc_node_candidates.end(), parity_comp_node_id);
//             if (it != reloc_node_candidates.end())
//             {
//                 reloc_node_candidates.erase(it);
//             }
//         }
//         else
//         {
//             // if the node already stores a block, then we need to relocate the newly computed parity block to another node

//             // Strategy 1: randomly pick one node with minimum cost to compute
//             size_t pos = Utils::randomUInt(0, reloc_node_candidates.size() - 1, random_generator);
//             // uniform_int_distribution<size_t> dist(0, reloc_node_candidates.size() - 1);
//             // default_random_engine engine(stripes[0]->getId());
//             // pos = dist(engine);

//             // // Strategy 2: pick the first node with minimum cost to compute
//             // size_t pos = 0;

//             size_t parity_reloc_node_id = reloc_node_candidates[pos];

//             // mark the node as relocated with a block
//             relocated_blk_distribution[parity_reloc_node_id] = true;

//             // remove the node from candidate list
//             reloc_node_candidates.erase(reloc_node_candidates.begin() + pos);

//             // create a parity relocation task
//             vector<size_t> solution;
//             solution.push_back(INVALID_ID);           // stripe_id as invalid
//             solution.push_back(code.k_f + parity_id); // new block_id (k_f + parity_id)
//             solution.push_back(parity_comp_node_id);  // from node_id
//             solution.push_back(parity_reloc_node_id); // to node_id

//             solutions.push_back(solution);
//         }
//     }

//     // printf("node candidates for block relocation (after PM):\n");
//     // Utils::printUIntVector(reloc_node_candidates);

//     // Step 2: data relocation

//     // record data blocks that needs relocation (stripe_id, block_id)
//     vector<pair<size_t, size_t>> data_blocks_to_reloc;

//     // check the number of data blocks on each node
//     vector<size_t> node_data_blk_dist(num_nodes, 0);
//     for (size_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++)
//     {
//         vector<size_t> &stripe_indices = stripes[stripe_id]->getStripeIndices();
//         for (size_t block_id = 0; block_id < code.k_i; block_id++)
//         {
//             size_t node_id = stripe_indices[block_id];
//             node_data_blk_dist[node_id]++;

//             // if the node already stores more than lambda_f = 1 data blocks, we need to relocate data blocks (start from the 2nd one)
//             if (node_data_blk_dist[node_id] > code.lambda_f)
//             {
//                 data_blocks_to_reloc.push_back(pair<size_t, size_t>(stripe_id, block_id));
//             }
//         }
//     }

//     // create data relocation task for each data block
//     for (auto &item : data_blocks_to_reloc)
//     {
//         size_t stripe_id = item.first;
//         size_t block_id = item.second;

//         vector<size_t> &stripe_indices = stripes[stripe_id]->getStripeIndices();
//         size_t from_node_id = stripe_indices[block_id];

//         // Strategy 1: randomly pick a node from data relocation candidates
//         size_t pos = Utils::randomUInt(0, reloc_node_candidates.size() - 1, random_generator);

//         // uniform_int_distribution<size_t> dist(0, reloc_node_candidates.size() - 1);
//         // default_random_engine engine(stripe_id);
//         // size_t pos = dist(engine);

//         // // Strategy 2: pick the first node from data relocation candidates
//         // size_t pos = 0;

//         size_t reloc_node_id = reloc_node_candidates[pos];

//         // mark the block as relocated
//         relocated_blk_distribution[reloc_node_id] = true;

//         // remove the element from data relocation candidate
//         reloc_node_candidates.erase(reloc_node_candidates.begin() + pos);

//         // create a data relocation task
//         vector<size_t> solution;
//         solution.push_back(stripe_id);
//         solution.push_back(block_id);
//         solution.push_back(from_node_id);
//         solution.push_back(reloc_node_id);

//         solutions.push_back(solution);
//     }

//     // printf("node candidates for block relocation (after PM + DM):\n");
//     // Utils::printUIntVector(reloc_node_candidates);
// }