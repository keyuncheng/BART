#include "RecvBipartite.hh"

RecvBipartite::RecvBipartite()
{
}

RecvBipartite::~RecvBipartite()
{
}

void RecvBipartite::print_block_metastore() {

}

size_t RecvBipartite::createBlockMeta(BlockMeta &in_block_meta) {

    // stripe group id must be valid
    if (in_block_meta.stripe_group_id != -1) {
        printf("invalid stripe_group %d\n", in_block_meta.stripe_group_id);
        return INVALID_ID;
    }

    // add to block_metastore
    block_metastore.push_back(in_block_meta);
    // update id
    int block_meta_id = (int) block_metastore.size() - 1;
    BlockMeta &block_meta = block_metastore[block_meta_id];
    block_meta.id = block_meta_id;
    
    // create records in stripe group for quicker search
    vector<BlockMeta *> &sg_block_metas = sg_block_meta_map[in_block_meta.stripe_group_id];
    sg_block_metas.push_back(&block_meta);

    return block_meta.id;
}

size_t RecvBipartite::findBlockMeta(BlockMeta &in_block_meta) {
    // stripe group id must be valid
    if (in_block_meta.stripe_group_id != -1) {
        printf("invalid stripe_group %d\n", in_block_meta.stripe_group_id);
        return INVALID_ID;
    }

    // find the block in the stripe group with the same block_id and type
    vector<BlockMeta *> &sg_block_metas = sg_block_meta_map[in_block_meta.stripe_group_id];
    for (auto &block_meta_ptr : sg_block_metas) {
        if (block_meta_ptr->block_id == in_block_meta.block_id && block_meta_ptr->type == in_block_meta.type) {
            return block_meta_ptr->id;
        }
    }

    return INVALID_ID;
}

size_t RecvBipartite::createBlockVtx(BlockMeta &in_block_meta) {
    //TODO
}

size_t RecvBipartite::createNodeVtx(int node_id) {

}


bool RecvBipartite::constructStripeBatchWithApproaches(StripeBatch &stripe_batch, vector<int> &approaches) {

    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    int num_stripe_groups = stripe_groups.size();
    if ((size_t) num_stripe_groups != approaches.size()) {
        printf("invalid parameters\n");
        return false;
    }

    // construct stripe group with approach
    for (int sg_id = 0; sg_id < num_stripe_groups; sg_id++) {
        if (constructStripeGroupWithApproach(stripe_groups[sg_id], approaches[sg_id]) == false) {
            return false;
        }
    }

    return true;
}

bool RecvBipartite::constructStripeGroupWithApproach(StripeGroup &stripe_group, int approach) {

    // construct bipartite graph with compute blocks (re-encoding / parity merging) and data / parity blocks for relocation
    if (approach == TransApproach::RE_ENCODE) {
        if (constructSGWithReEncoding(stripe_group) == false) {
            return false;
        }
    } else if (approach == TransApproach::PARITY_MERGE) {
        if (constructSGWithParityMerging(stripe_group) == false) {
            return false;
        }
    }

    // construct with parity block
    if (constructSGWithParityRelocation(stripe_group) == false) {
        return false;
    }

    // construct with data block
    if (constructSGWithData(stripe_group) == false) {
        return false;
    }

    return true;
}

bool RecvBipartite::constructSGWithReEncoding(StripeGroup &stripe_group) {
    // create a compute block (on left) to represent the dummy block for re-encoding computation
    
    ConvertibleCode &code = stripe_group.getCode();
    ClusterSettings &settings = stripe_group.getClusterSettings();
    int num_nodes = settings.M;
    vector<int> data_distribution = stripe_group.getDataDistribution();

    // candidate nodes for parity blocks computation, where we need to collect k_f data blocks (all nodes are candidates)
    vector<int> parity_comp_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        parity_comp_candidates.push_back(node_id);
    }

    // create metadata for the parity compute block
    BlockMeta in_block_meta;
    in_block_meta.type = COMPUTE_BLK_RE;
    in_block_meta.stripe_group_id = stripe_group.getId();
    size_t block_meta_id = createBlockMeta(in_block_meta);

    // add vertex of the parity compute block 
    size_t pcb_vtx_id = addVertex(VertexType::LEFT);

    // create mapping
    block_meta_to_vtx_map[block_meta_id] = pcb_vtx_id;
    vtx_to_block_meta_map[pcb_vtx_id] = block_meta_id;
    
    // get parity compute block vertex
    Vertex &pcb_vtx = vertices[pcb_vtx_id];
    
    // create edges to all candidate nodes for re-encoding computation. On a node with x data blocks, the edge is with cost (k_f - x)
    for (auto cand_node_id : parity_comp_candidates) {
        // get node vertex
        size_t nvtx_id = INVALID_ID;
        auto it = node_to_vtx_map.find(cand_node_id);
        if ( == )
        Vertex &nvtx = *getNodeVtx(node_meta);

        // add edge
        Edge edge = {
            .id = (int) edges_map.size(),
            .lvtx = &cb_vtx,
            .rvtx = &nvtx,
            .weight = 1,
            .cost = code.k_f - data_distribution[cand_node_id], // cost: # of required data blocks
        };
        edges_map[edge.id] = edge;

        // add to vtx2edge map
        lvtx_edges_map[cb_vtx.id].push_back(edge.id);
        rvtx_edges_map[nvtx.id].push_back(edge.id);
    }

    return true;
}

bool RecvBipartite::constructSGWithParityMerging(StripeGroup &stripe_group) {
    // create m_f compute blocks (on left) to represent the dummy blocks for parity merging computation

    ConvertibleCode &code = stripe_group.getCode();
    ClusterSettings &settings = stripe_group.getClusterSettings();
    int num_nodes = settings.M;

    // get parity block distributions
    vector<vector<int> > parity_distributions = stripe_group.getParityDistributions();

    // candidate nodes for parity blocks computation, where we need to collect alpha parity blocks with the same offset (all nodes are candidates)
    vector<int> parity_comp_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        parity_comp_candidates.push_back(node_id);
    }

    // for each parity block, generate edges to all candidate nodes
    for (int parity_id = 0; parity_id < code.m_f; parity_id++) {
        vector<int> &parity_distribution = parity_distributions[parity_id];

        // get vertex of the parity compute block
        BlockMeta block_meta = {
            .id = -1,
            .type = COMPUTE_BLK_PM, // compute block with parity merging
            .stripe_batch_id = -1,
            .stripe_group_id = stripe_group.getId(),
            .stripe_id_global = -1,
            .stripe_id = -1,
            .block_id = code.k_f + parity_id, // new parity block id
            .vtx_id = -1
        };
        Vertex &bvtx = *getBlockVtx(block_meta);

        for (auto cand_node_id : parity_comp_candidates) {
            // get node vertex
            NodeMeta node_meta = {
                .id = -1,
                .node_id = cand_node_id,
                .vtx_id = -1
            };
            Vertex &nvtx = *getNodeVtx(node_meta);

            // add edge
            Edge edge = {
                .id = (int) edges_map.size(),
                .lvtx = &bvtx,
                .rvtx = &nvtx,
                .weight = 1,
                .cost = code.alpha - parity_distribution[cand_node_id], // cost: # of required parity blocks
            };
            edges_map[edge.id] = edge;

            // add to vtx2edge map
            lvtx_edges_map[bvtx.id].push_back(edge.id);
            rvtx_edges_map[nvtx.id].push_back(edge.id);
        }
    }

    return true;
}

bool RecvBipartite::constructSGWithParityRelocation(StripeGroup &stripe_group) {
    // create m_f parity blocks (on left) to represent newly computed parity block relocation

    ConvertibleCode &code = stripe_group.getCode();
    ClusterSettings &cluster_settings = stripe_group.getClusterSettings();
    int num_nodes = cluster_settings.M;

    vector<int> data_distribution = stripe_group.getDataDistribution();

    // candidate nodes for m_f new parity blocks relocation (requirement: no data block stored on the node)
    vector<int> parity_reloc_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (data_distribution[node_id] == 0) {
            parity_reloc_candidates.push_back(node_id);
        }
    }

    for (int parity_id = 0; parity_id < code.m_f; parity_id++) {
        // get vertex of the new parity block 
        BlockMeta block_meta = {
            .id = -1,
            .type = PARITY_BLK, // parity block (for relocation)
            .stripe_batch_id = -1,
            .stripe_group_id = stripe_group.getId(),
            .stripe_id_global = -1,
            .stripe_id = -1,
            .block_id = code.k_f + parity_id, // new parity block id
            .vtx_id = -1
        };
        Vertex &bvtx = *getBlockVtx(block_meta);

        for (auto cand_node_id : parity_reloc_candidates) {
            // get node vertex
            NodeMeta node_meta = {
                .id = -1,
                .node_id = cand_node_id,
                .vtx_id = -1
            };
            Vertex &nvtx = *getNodeVtx(node_meta);

            // add edge
            Edge edge = {
                .id = (int) edges_map.size(),
                .lvtx = &bvtx,
                .rvtx = &nvtx,
                .weight = 1,
                .cost = 1 // cost: block relocation
            };
            edges_map[edge.id] = edge;

            // add to vtx2edge map
            lvtx_edges_map[bvtx.id].push_back(edge.id);
            rvtx_edges_map[nvtx.id].push_back(edge.id);
        }
    }

    return true;
}

bool RecvBipartite::constructSGWithData(StripeGroup &stripe_group) {

    ConvertibleCode &code = stripe_group.getCode();
    ClusterSettings &settings = stripe_group.getClusterSettings();
    int num_nodes = settings.M;
    vector<Stripe *> &stripes = stripe_group.getStripes();

    // get data distribution
    vector<int> data_distribution = stripe_group.getDataDistribution();

    // candidate nodes for data relocation (requirement: data block stored on the node is less than lambda_f)
    vector<int> data_reloc_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (data_distribution[node_id] < code.lambda_f) {
            data_reloc_candidates.push_back(node_id);
        }
    }

    // get data blocks to relocate
    vector<pair<int, int> > data_blocks_to_reloc;

    // if a node already stores more than lambda_f data blocks, relocate those data blocks
    vector<int> node_data_blk_dist(num_nodes, 0);
    for (int stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
        vector<int> &stripe_indices = stripes[stripe_id]->getStripeIndices();
        for (int block_id = 0; block_id < code.k_i; block_id++) {
            int node_id = stripe_indices[block_id];
            node_data_blk_dist[node_id]++;

            if (node_data_blk_dist[node_id] > code.lambda_f) {
                data_blocks_to_reloc.push_back(pair<int, int>(stripe_id, block_id));
            }
        }
    }

    // create vertices and edges for the data blocks to be relocated
    for (auto &item : data_blocks_to_reloc) {
        int stripe_id = item.first;
        int block_id = item.second;

        // get data block vertex
        BlockMeta block_meta = {
            .id = -1,
            .type = DATA_BLK,
            .stripe_batch_id = -1,
            .stripe_group_id = stripe_group.getId(),
            .stripe_id_global = stripes[stripe_id]->getId(),
            .stripe_id = stripe_id,
            .block_id = block_id,
            .vtx_id = -1
        };
        Vertex &bvtx = *getBlockVtx(block_meta);

        for (auto cand_node_id : data_reloc_candidates) {
            // get node vertex
            NodeMeta node_meta = {
                .id = -1,
                .node_id = cand_node_id,
                .vtx_id = -1
            };
            Vertex &nvtx = *getNodeVtx(node_meta);

            // add edge (bvtx -> nvtx)
            Edge edge = {
                .id = (int) edges_map.size(),
                .lvtx = &bvtx,
                .rvtx = &nvtx,
                .weight = 1,
                .cost = 1  // cost: block relocation
            };
            edges_map[edge.id] = edge;

            // add to vtx2edge map
            lvtx_edges_map[bvtx.id].push_back(edge.id);
            rvtx_edges_map[nvtx.id].push_back(edge.id);
        }
    }

    return true;
}


bool RecvBipartite::findEdgesWithApproachesGreedy(StripeBatch &stripe_batch, vector<int> &edges, mt19937 &random_generator) {
    // key idea: for each block, find a node with **minimum load after connection** among all possible candidates; if more than one node have the same minimum load, randomly pick one

    ClusterSettings &settings = stripe_batch.getClusterSettings();
    int num_nodes = settings.M;

    // initialize the chosen edges
    edges.clear();

    // for each stripe group, record the nodes relocated with a block (to avoid a node overlapped with a data and parity block)
    unordered_map<int, vector<bool> > sg_reloc_nodes_map; // <stripe_group_id, <node_ids relocated with blocks> >
    for (auto it = block_meta_map.begin(); it != block_meta_map.end(); it++) {
        BlockMeta &block_meta = it->second;
        if (sg_reloc_nodes_map.find(block_meta.stripe_group_id) == sg_reloc_nodes_map.end()) {
            // init all nodes are not relocated
            sg_reloc_nodes_map[block_meta.stripe_group_id] = vector<bool>(num_nodes, false);
        }
    }

    // find solution for each block on the left
    vector<int> lvtx_ids;
    for (auto lvit : left_vertices_map) {
        lvtx_ids.push_back(lvit.first);
    }
    sort(lvtx_ids.begin(), lvtx_ids.end());

    for (auto lvtx_id : lvtx_ids) {
        BlockMeta &block_meta = *getBlockMeta(lvtx_id);
        vector<bool> &sg_relocated_nodes = sg_reloc_nodes_map[block_meta.stripe_group_id];

        // find candidate edges from adjacent edges
        vector<int> cand_edges;
        vector<int> cand_costs;

        // if the block is a data / parity block (for relocation), filter out edges that connect to a node relocated with a data / parity block
        vector<int> &adjacent_edges = lvtx_edges_map[lvtx_id];
        for (auto edge_id : adjacent_edges) {
            Edge &edge = edges_map[edge_id];
            NodeMeta &node_meta = *getNodeMeta(edge.rvtx->id);
            if (block_meta.type == DATA_BLK || block_meta.type == PARITY_BLK) {
                if (sg_relocated_nodes[node_meta.node_id] == true) {
                    continue;
                }
            }

            cand_edges.push_back(edge.id);
            cand_costs.push_back(edge.rvtx->costs + edge.cost); // cost after edge connection
        }

        // find all min_load_edge candidates (with the same recv load)
        vector<int> sorted_idxes = Utils::argsortIntVector(cand_costs);
        int min_load = cand_costs[sorted_idxes[0]];
        vector<int> cand_min_load_edges;
        for (auto idx : sorted_idxes) {
            if (cand_costs[idx] == min_load) {
                cand_min_load_edges.push_back(cand_edges[idx]);
            } else {
                break;
            }
        }

        // randomly pick one min-load edge
        int rand_pos = Utils::randomInt(0, cand_min_load_edges.size() - 1, random_generator);
        int min_load_edge_id = cand_min_load_edges[rand_pos];
        edges.push_back(min_load_edge_id); // add edge id

        // if the block is a data / parity block, mark the node as relocated
        if (block_meta.type == DATA_BLK || block_meta.type == PARITY_BLK) {
            Edge &min_load_edge = edges_map[min_load_edge_id];
            NodeMeta &min_load_node_meta = *getNodeMeta(min_load_edge.rvtx->id);
            sg_relocated_nodes[min_load_node_meta.node_id] = true;
        }
    }

    return true;
}

bool RecvBipartite::constructPartialSolutionFromEdges(StripeBatch &stripe_batch, vector<int> &edges, vector<vector<int> > &partial_solutions) {
    /** construct partial solutions from recv graph
     * Currently, we have no information about send load distribution; 
     * thus, we first construct partial solutions from recv graph with edges, set from_node to -1;
     * In send graph, we finalize the send distribution
    **/

    for (auto edge_id : edges) {
        Edge &edge = edges_map[edge_id];
        BlockMeta &block_meta = *getBlockMeta(edge.lvtx->id);
        NodeMeta &node_meta = *getNodeMeta(edge.rvtx->id);

        // add partial solution: <sg_id, block_type, stripe_id, block_id, from_node, to_node>
        vector<int> partial_solution;
        partial_solution.push_back(block_meta.stripe_group_id);
        partial_solution.push_back(block_meta.type);
        partial_solution.push_back(block_meta.stripe_id);
        partial_solution.push_back(block_meta.block_id);
        partial_solution.push_back(-1); // from node temporarily set to -1
        partial_solution.push_back(node_meta.node_id);

        partial_solutions.push_back(partial_solution);
    }

    return true;
}

bool RecvBipartite::updatePartialSolutionFromRecvGraph(StripeBatch &stripe_batch, vector<vector<int> > &partial_solutions, vector<vector<int> > &solutions) {
    // construct solution <sg_id, block_type, stripe_id, block_id, from_node, to_node> from recv graph
    // input: in each of solution from recv graph, we set from_node to -1, then we make it initialized in send graph
    // output: <stripe_id, block_id, from_node, to_node>

    ConvertibleCode &code = stripe_batch.getCode();
    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    // stripe group parity block compute map
    unordered_map<int, vector<int> > sg_data_distribution_map;
    unordered_map<int, vector<int> > sg_parity_comp_node_map;
    unordered_map<int, vector<pair<int, int> > > sg_block_swap_map; // <stripe group id, <from_node, to_node> >
    for (auto &stripe_group : stripe_groups) {
        sg_data_distribution_map[stripe_group.getId()] = stripe_group.getDataDistribution();
        sg_parity_comp_node_map[stripe_group.getId()] = vector<int>(code.m_f, -1); // for each parity block, mark from_node as -1
        sg_block_swap_map[stripe_group.getId()].clear();
    }


    // initialize solutions
    solutions.clear();

    for (auto &partial_solution : partial_solutions) {
        // construct solution <sg_id, block_type, stripe_id, block_id, from_node, to_node, load>
        int sg_id = partial_solution[0];
        int block_type = partial_solution[1];
        int stripe_id = partial_solution[2];
        int block_id = partial_solution[3];
        int from_node_id = partial_solution[4];
        int to_node_id = partial_solution[5];

        StripeGroup &stripe_group = stripe_groups[sg_id];

        // get parity block compute
        vector<int> &sg_parity_comp_nodes = sg_parity_comp_node_map[sg_id];

        if (block_type == COMPUTE_BLK_RE) {
            // for each data block not stored at to_node_id, create a data block relocation task
            for (int stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
                Stripe &stripe = *stripe_group.getStripes()[stripe_id];
                vector<int> &stripe_indices = stripe.getStripeIndices();

                for (int db_blk_id = 0; db_blk_id < code.k_i; db_blk_id++) {
                    int data_blk_from_node_id = stripe_indices[db_blk_id];
                    if (data_blk_from_node_id != to_node_id) {
                        // add solution
                        vector<int> solution;
                        solution.push_back(stripe_id);
                        solution.push_back(db_blk_id);
                        solution.push_back(data_blk_from_node_id);
                        solution.push_back(to_node_id);

                        solutions.push_back(solution);
                    }
                }
            }

            // mark in parity block compute map (parity computation is at to_node_id)
            for (int parity_id = 0; parity_id < code.m_f; parity_id++) {
                sg_parity_comp_nodes[parity_id] = to_node_id;
            }

        } else if (block_type == COMPUTE_BLK_PM) {
            // for each parity block parity_id not stored at to_node_id, create a parity block relocation task
            
            int parity_id = block_id - code.k_f;
            for (int stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
                Stripe &stripe = *stripe_group.getStripes()[stripe_id];
                vector<int> &stripe_indices = stripe.getStripeIndices();
                int parity_blk_from_node_id = stripe_indices[code.k_i + parity_id];

                if (parity_blk_from_node_id != to_node_id) {
                    // add solution
                    vector<int> solution;
                    solution.push_back(stripe_id);
                    solution.push_back(code.k_i + parity_id);
                    solution.push_back(parity_blk_from_node_id);
                    solution.push_back(to_node_id);

                    solutions.push_back(solution);
                }
            }

            // mark in parity block compute map (parity computation is at to_node_id)
            sg_parity_comp_nodes[parity_id] = to_node_id;
        } else if (block_type == PARITY_BLK) {
            int parity_id = block_id - code.k_f;
            int parity_from_node_id = sg_parity_comp_nodes[parity_id];

            if (parity_from_node_id == -1) {
                printf("error: invalid from_id %d for parity_id: %d\n", parity_from_node_id, parity_id);
            }

            // optimization: if the parity compute node does not store a data block, we can relocate the parity block at the compute node, thus no need to add a relocation task
            // we create a block swap task
            vector<int> &data_distribution = sg_data_distribution_map[sg_id];
            if (data_distribution[parity_from_node_id] == 0) {
                vector<pair<int, int> > &sg_block_swap_tasks = sg_block_swap_map[sg_id];
                sg_block_swap_tasks.push_back(pair<int, int>(parity_from_node_id, to_node_id));
            } else {
                // add solution
                vector<int> solution;
                solution.push_back(stripe_id);
                solution.push_back(block_id);
                solution.push_back(parity_from_node_id);
                solution.push_back(to_node_id);

                solutions.push_back(solution);
            }
        } else if (block_type == DATA_BLK) {
            Stripe &stripe = *stripe_group.getStripes()[stripe_id];
            vector<int> &stripe_indices = stripe.getStripeIndices();
            from_node_id = stripe_indices[block_id];

            // first check if there is a block swap task
            vector<pair<int, int> > &sg_block_swap_tasks = sg_block_swap_map[sg_id];
            for (auto it = sg_block_swap_tasks.begin(); it != sg_block_swap_tasks.end(); it++) {
                // change the to_node_id to from_node_id in block swap task, then remove the block swap task
                int task_from_node = it->first;
                int task_to_node = it->second;
                if (task_to_node == to_node_id) {
                    to_node_id = task_from_node;
                    sg_block_swap_tasks.erase(it);
                    break;
                }
            }

            // add solution
            vector<int> solution;
            solution.push_back(stripe_id);
            solution.push_back(block_id);
            solution.push_back(from_node_id);
            solution.push_back(to_node_id);

            solutions.push_back(solution);

        } else {
            printf("error: invalid block type\n");
            return false;
        }
    }

    return true;
}