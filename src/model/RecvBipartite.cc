#include "RecvBipartite.hh"

RecvBipartite::RecvBipartite()
{
}

RecvBipartite::~RecvBipartite()
{
}

void RecvBipartite::print_block_metastore() {
    printf("Block Metastore:\n");
    for (auto it = sg_block_meta_map.begin(); it != sg_block_meta_map.end(); it++) {
        size_t stripe_group_id = it->first;
        vector<BlockMeta *> &sg_block_meta = it->second;

        printf("stripe_group: %ld\n", stripe_group_id);
        for (auto block_meta_ptr : sg_block_meta) {
            BlockMeta &block_meta = *block_meta_ptr;
            printf("id: %ld, type: %d, vtx_id: %ld, stripe_batch_id: %ld, stripe_group_id: %ld, stripe_id: %ld, stripe_id (global): %ld, block_id: %ld\n", block_meta.id, block_meta.type, block_meta.vtx_id, block_meta.stripe_batch_id, block_meta.stripe_group_id, block_meta.stripe_id, block_meta.stripe_id_global, block_meta.block_id);
        }
    }
}

size_t RecvBipartite::createBlockMeta(BlockMeta &in_block_meta) {

    // stripe group id must be valid
    if (in_block_meta.stripe_group_id == INVALID_ID) {
        printf("invalid stripe_group %ld\n", in_block_meta.stripe_group_id);
        return INVALID_ID;
    }

    // add to block_metastore
    size_t new_block_meta_id = block_metastore.size();
    block_metastore[new_block_meta_id] = in_block_meta;
    BlockMeta &block_meta = block_metastore[new_block_meta_id];
    // update id
    block_meta.id = new_block_meta_id;
    
    // create records in stripe group for quicker search
    vector<BlockMeta *> &sg_block_metas = sg_block_meta_map[in_block_meta.stripe_group_id];
    sg_block_metas.push_back(&block_meta);

    return block_meta.id;
}

size_t RecvBipartite::findBlockMeta(BlockMeta &in_block_meta) {
    // stripe group id must be valid
    if (in_block_meta.stripe_group_id == INVALID_ID) {
        printf("invalid stripe_group %ld\n", in_block_meta.stripe_group_id);
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
    // all blocks (and metadata) are newly created, no need to call findBlockMeta()
    
    // create block meta
    size_t block_meta_id = createBlockMeta(in_block_meta);

    // add vertex of the block 
    size_t block_vtx_id = addVertex(VertexType::LEFT);

    BlockMeta &block_meta = block_metastore[block_meta_id];
    block_meta.vtx_id = block_vtx_id;

    // create mapping
    block_meta_to_vtx_map[block_meta_id] = block_vtx_id;
    vtx_to_block_meta_map[block_vtx_id] = block_meta_id;

    return block_vtx_id;
}

size_t RecvBipartite::createNodeVtx(size_t node_id) {
    if (node_to_vtx_map.find(node_id) != node_to_vtx_map.end()) {
        return node_to_vtx_map[node_id];
    } else {

        // add vertex of the parity compute block 
        size_t node_vtx_id = addVertex(VertexType::RIGHT);
        
        node_to_vtx_map[node_id] = node_vtx_id;
        vtx_to_node_map[node_vtx_id] = node_id;

        return node_vtx_id;
    }
}

bool RecvBipartite::constructStripeBatchWithApproaches(StripeBatch &stripe_batch, vector<size_t> &approaches) {

    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    size_t num_stripe_groups = stripe_groups.size();
    if (num_stripe_groups != approaches.size()) {
        printf("invalid parameters\n");
        return false;
    }

    // construct stripe group with approach
    for (size_t sg_id = 0; sg_id < num_stripe_groups; sg_id++) {
        if (constructStripeGroupWithApproach(stripe_groups[sg_id], approaches[sg_id]) == false) {
            return false;
        }
    }

    return true;
}

bool RecvBipartite::constructStripeGroupWithApproach(StripeGroup &stripe_group, size_t approach) {

    // construct bipartite graph with compute blocks (re-encoding / parity merging) and data / parity blocks for relocation
    if (approach == (size_t) TransApproach::RE_ENCODE) {
        if (constructSGWithReEncoding(stripe_group) == false) {
            return false;
        }
    } else if (approach == (size_t) TransApproach::PARITY_MERGE) {
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
    size_t num_nodes = settings.M;
    vector<size_t> data_distribution = stripe_group.getDataDistribution();

    // candidate nodes for parity blocks computation, where we need to collect k_f data blocks (all nodes are candidates)
    vector<size_t> parity_comp_candidates;
    for (size_t node_id = 0; node_id < num_nodes; node_id++) {
        parity_comp_candidates.push_back(node_id);
    }

    // create parity compute block vertex
    BlockMeta in_block_meta;
    in_block_meta.type = COMPUTE_BLK_RE;
    in_block_meta.stripe_group_id = stripe_group.getId();
    size_t pcb_vtx_id = createBlockVtx(in_block_meta);
    
    // create edges to all candidate nodes for re-encoding computation. On a node with x data blocks, the edge is with cost (k_f - x)
    for (auto cand_node_id : parity_comp_candidates) {
        Vertex &pcb_vtx = vertices_map[pcb_vtx_id];

        // get node vertex
        size_t node_vtx_id = createNodeVtx(cand_node_id);
        Vertex &node_vtx = vertices_map[node_vtx_id];

        // add edge (cost: # of required data blocks)
        addEdge(&pcb_vtx, &node_vtx, 1, code.k_f - data_distribution[cand_node_id]);
    }

    return true;
}

bool RecvBipartite::constructSGWithParityMerging(StripeGroup &stripe_group) {
    // create m_f compute blocks (on left) to represent the dummy blocks for parity merging computation

    ConvertibleCode &code = stripe_group.getCode();
    ClusterSettings &settings = stripe_group.getClusterSettings();
    size_t num_nodes = settings.M;

    // get data block distributions
    vector<size_t> data_distribution = stripe_group.getDataDistribution();

    // get parity block distributions
    vector<vector<size_t> > parity_distributions = stripe_group.getParityDistributions();

    // candidate nodes for parity blocks computation, where we need to collect alpha parity blocks with the same offset (all nodes are candidates)
    vector<size_t> parity_comp_candidates;
    for (size_t node_id = 0; node_id < num_nodes; node_id++) {
        parity_comp_candidates.push_back(node_id);
    }

    // for each parity block, generate edges to all candidate nodes
    for (size_t parity_id = 0; parity_id < code.m_f; parity_id++) {
        vector<size_t> &parity_distribution = parity_distributions[parity_id];

        // create parity compute block vertex
        BlockMeta in_block_meta;
        in_block_meta.type = COMPUTE_BLK_PM;
        in_block_meta.stripe_group_id = stripe_group.getId();
        in_block_meta.block_id = code.k_f + parity_id; // new parity block id
        size_t pcb_vtx_id = createBlockVtx(in_block_meta);
        Vertex &pcb_vtx = vertices_map[pcb_vtx_id];

        for (auto cand_node_id : parity_comp_candidates) {
            // get node vertex
            size_t node_vtx_id = createNodeVtx(cand_node_id);
            Vertex &node_vtx = vertices_map[node_vtx_id];

            // manual tune cost (if there is a data block at the node, need to relocate to another node, thus this node is with higher cost than the others)
            int edge_cost = code.alpha - parity_distribution[cand_node_id];
            if (data_distribution[cand_node_id] > 0) {
                edge_cost += 1;
            }

            // add edge (cost: # of required parity blocks)
            addEdge(&pcb_vtx, &node_vtx, 1, edge_cost);
        }
    }

    return true;
}

bool RecvBipartite::constructSGWithParityRelocation(StripeGroup &stripe_group) {
    // create m_f parity blocks (on left) to represent newly computed parity block relocation

    ConvertibleCode &code = stripe_group.getCode();
    ClusterSettings &cluster_settings = stripe_group.getClusterSettings();
    size_t num_nodes = cluster_settings.M;

    vector<size_t> data_distribution = stripe_group.getDataDistribution();

    // candidate nodes for m_f new parity blocks relocation (requirement: no data block stored on the node)
    vector<size_t> parity_reloc_candidates;
    for (size_t node_id = 0; node_id < num_nodes; node_id++) {
        if (data_distribution[node_id] == 0) {
            parity_reloc_candidates.push_back(node_id);
        }
    }

    for (size_t parity_id = 0; parity_id < code.m_f; parity_id++) {
        // create parity compute block vertex
        BlockMeta in_block_meta;
        in_block_meta.type = PARITY_BLK;
        in_block_meta.stripe_group_id = stripe_group.getId();
        in_block_meta.block_id = code.k_f + parity_id; // new parity block id
        size_t prb_vtx_id = createBlockVtx(in_block_meta);
        Vertex &prb_vtx = vertices_map[prb_vtx_id];

        for (auto cand_node_id : parity_reloc_candidates) {
            // get node vertex
            size_t node_vtx_id = createNodeVtx(cand_node_id);
            Vertex &node_vtx = vertices_map[node_vtx_id];

            // add edge (cost: 1 for block relocation)
            addEdge(&prb_vtx, &node_vtx, 1, 1);
        }
    }

    return true;
}

bool RecvBipartite::constructSGWithData(StripeGroup &stripe_group) {

    ConvertibleCode &code = stripe_group.getCode();
    ClusterSettings &settings = stripe_group.getClusterSettings();
    size_t num_nodes = settings.M;
    vector<Stripe *> &stripes = stripe_group.getStripes();

    // get data distribution
    vector<size_t> data_distribution = stripe_group.getDataDistribution();

    // candidate nodes for data relocation (requirement: data block stored on the node is less than lambda_f)
    vector<size_t> data_reloc_candidates;
    for (size_t node_id = 0; node_id < num_nodes; node_id++) {
        if (data_distribution[node_id] < code.lambda_f) {
            data_reloc_candidates.push_back(node_id);
        }
    }

    // get data blocks to relocate
    vector<pair<size_t, size_t> > data_blocks_to_reloc;

    // if a node already stores more than lambda_f data blocks, relocate those data blocks
    vector<size_t> node_data_blk_dist(num_nodes, 0);
    for (size_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
        vector<size_t> &stripe_indices = stripes[stripe_id]->getStripeIndices();
        for (size_t block_id = 0; block_id < code.k_i; block_id++) {
            size_t node_id = stripe_indices[block_id];
            node_data_blk_dist[node_id]++;

            if (node_data_blk_dist[node_id] > code.lambda_f) {
                data_blocks_to_reloc.push_back(pair<size_t, size_t>(stripe_id, block_id));
            }
        }
    }

    // create vertices and edges for the data blocks to be relocated
    for (auto &item : data_blocks_to_reloc) {
        size_t stripe_id = item.first;
        size_t block_id = item.second;

        // create parity compute block vertex
        BlockMeta in_block_meta;
        in_block_meta.type = DATA_BLK;
        in_block_meta.stripe_group_id = stripe_group.getId();
        in_block_meta.stripe_id_global = stripes[stripe_id]->getId();
        in_block_meta.stripe_id = stripe_id;
        in_block_meta.block_id = block_id; // new parity block id
        size_t db_vtx_id = createBlockVtx(in_block_meta);
        Vertex &db_vtx = vertices_map[db_vtx_id];

        for (auto cand_node_id : data_reloc_candidates) {
            // get node vertex
            size_t node_vtx_id = createNodeVtx(cand_node_id);
            Vertex &node_vtx = vertices_map[node_vtx_id];

            // add edge (cost: 1 for block relocation)
            addEdge(&db_vtx, &node_vtx, 1, 1);
        }
    }

    return true;
}

bool RecvBipartite::findEdgesWithApproachesGreedy(StripeBatch &stripe_batch, vector<size_t> &sol_edges, mt19937 &random_generator) {
    // key idea: for each block, find a node with **minimum load after connection** among all possible candidates; if more than one node have the same minimum load, randomly pick one

    ClusterSettings &settings = stripe_batch.getClusterSettings();
    size_t num_nodes = settings.M;

    // initialize the solution edges
    sol_edges.clear();

    // for each stripe group, record the nodes relocated with a block (to avoid a node overlapped with a data and parity block)
    unordered_map<size_t, vector<bool> > sg_reloc_nodes_map; // <stripe_group_id, <node_ids relocated with blocks> >

    for (auto it = sg_block_meta_map.begin(); it != sg_block_meta_map.end(); it++) {
        sg_reloc_nodes_map[it->first] = vector<bool>(num_nodes, false); // init all nodes are not relocated
    }

    // find solution for each block on the left
    vector<size_t> lvtx_ids;
    for (auto lvit : left_vertices_map) {
        lvtx_ids.push_back(lvit.first);
    }
    sort(lvtx_ids.begin(), lvtx_ids.end());

    for (auto lvtx_id : lvtx_ids) {
        BlockMeta &block_meta = block_metastore[vtx_to_block_meta_map[lvtx_id]];
        vector<bool> &sg_relocated_nodes = sg_reloc_nodes_map[block_meta.stripe_group_id];

        // find candidate edges from adjacent edges
        vector<size_t> cand_edges;
        vector<int> cand_costs;

        // if the block is a data / parity block (for relocation), filter out edges that connect to a node relocated with a data / parity block
        vector<size_t> &adjacent_edges = lvtx_edges_map[lvtx_id];
        for (auto edge_id : adjacent_edges) {
            Edge &edge = edges_map[edge_id];
            size_t node_id = vtx_to_node_map[edge.rvtx->id];
            if (block_meta.type == DATA_BLK || block_meta.type == PARITY_BLK) {
                if (sg_relocated_nodes[node_id] == true) {
                    continue;
                }
            }

            cand_edges.push_back(edge.id);
            cand_costs.push_back(edge.rvtx->costs + edge.cost); // cost after edge connection
        }


        // find all min_load_edge candidates (with the same recv load)
        vector<size_t> sorted_idxes = Utils::argsortIntVector(cand_costs);
        int min_load = cand_costs[sorted_idxes[0]];
        vector<size_t> cand_min_load_edges;
        for (auto idx : sorted_idxes) {
            if (cand_costs[idx] == min_load) {
                cand_min_load_edges.push_back(cand_edges[idx]);
            } else {
                break;
            }
        }

        // randomly pick one min-load edge
        size_t rand_pos = Utils::randomUInt(0, cand_min_load_edges.size() - 1, random_generator);
        size_t min_load_edge_id = cand_min_load_edges[rand_pos];
        sol_edges.push_back(min_load_edge_id); // add edge id

        // update load on both vertex
        Edge &min_load_edge = edges_map[min_load_edge_id];
        Vertex &lvtx = vertices_map[lvtx_id];
        Vertex &rvtx = vertices_map[min_load_edge.rvtx->id];
        lvtx.out_degree += min_load_edge.weight;
        lvtx.costs += min_load_edge.cost;
        rvtx.out_degree += min_load_edge.weight;
        rvtx.costs += min_load_edge.cost;

        
        // if the block is a data / parity block, mark the node as relocated
        if (block_meta.type == DATA_BLK || block_meta.type == PARITY_BLK) {
            size_t min_load_node_id = vtx_to_node_map[min_load_edge.rvtx->id];
            sg_relocated_nodes[min_load_node_id] = true;
        }
    }

    return true;
}

bool RecvBipartite::findEdgesWithApproachesGreedySorted(StripeBatch &stripe_batch, vector<size_t> &sol_edges, mt19937 &random_generator) {
    // key idea: in addition to findEdgesWithApproachesGreedy, sort the block types as: compute blocks first; then relocation blocks

    ClusterSettings &settings = stripe_batch.getClusterSettings();
    size_t num_nodes = settings.M;

    // initialize the solution edges
    sol_edges.clear();

    // for each stripe group, record the nodes relocated with a block (to avoid a node overlapped with a data and parity block)
    unordered_map<size_t, vector<bool> > sg_reloc_nodes_map; // <stripe_group_id, <node_ids relocated with blocks> >

    for (auto it = sg_block_meta_map.begin(); it != sg_block_meta_map.end(); it++) {
        sg_reloc_nodes_map[it->first] = vector<bool>(num_nodes, false); // init all nodes are not relocated
    }

    // find solution for each block on the left
    vector<size_t> sorted_lvtx_ids;
    vector<size_t> compute_lvtx_ids; // compute lvtx
    vector<size_t> reloc_lvtx_ids; // relocate lvtx (data and parity)
    
    for (auto lvit : left_vertices_map) {
        Vertex &vtx = *lvit.second;
        BlockMeta &block_meta = block_metastore[vtx_to_block_meta_map[vtx.id]];

        if (block_meta.type == COMPUTE_BLK_RE || block_meta.type == COMPUTE_BLK_PM) {
            compute_lvtx_ids.push_back(vtx.id);
        } else {
            reloc_lvtx_ids.push_back(vtx.id);
        }
    }

    sort(compute_lvtx_ids.begin(), compute_lvtx_ids.end());
    sort(reloc_lvtx_ids.begin(), reloc_lvtx_ids.end());

    // first add compute vertex
    for (auto vtx_id : compute_lvtx_ids) {
        sorted_lvtx_ids.push_back(vtx_id);
    }

    // // next add relocate vertex
    // for (auto vtx_id : reloc_lvtx_ids) {
    //     sorted_lvtx_ids.push_back(vtx_id);
    // }

    for (auto lvtx_id : sorted_lvtx_ids) {
        BlockMeta &block_meta = block_metastore[vtx_to_block_meta_map[lvtx_id]];
        vector<bool> &sg_relocated_nodes = sg_reloc_nodes_map[block_meta.stripe_group_id];

        // find candidate edges from adjacent edges
        vector<size_t> cand_edges;
        vector<int> cand_costs;

        // if the block is a data / parity block (for relocation), filter out edges that connect to a node relocated with a data / parity block
        vector<size_t> &adjacent_edges = lvtx_edges_map[lvtx_id];
        for (auto edge_id : adjacent_edges) {
            Edge &edge = edges_map[edge_id];
            size_t node_id = vtx_to_node_map[edge.rvtx->id];
            if (block_meta.type == DATA_BLK || block_meta.type == PARITY_BLK) {
                if (sg_relocated_nodes[node_id] == true) {
                    continue;
                }
            }

            cand_edges.push_back(edge.id);
            cand_costs.push_back(edge.rvtx->costs + edge.cost); // cost after edge connection
        }

        // printf("block vtx_id: %ld, sg_id: %ld\n", block_meta.vtx_id, block_meta.stripe_group_id);
        // for (auto edge_id : cand_edges) {
        //     Edge &edge = edges_map[edge_id];
        //     printf("edge_id: %ld, rvtx_id: %ld, node_id: %ld, cost: %d\n", edge.id, edge.rvtx->id, vtx_to_node_map[edge.rvtx->id], edge.cost);
        // }


        // find all min_load_edge candidates (with the same recv load)
        vector<size_t> sorted_idxes = Utils::argsortIntVector(cand_costs);
        int min_load = cand_costs[sorted_idxes[0]];
        vector<size_t> cand_min_load_edges;
        for (auto idx : sorted_idxes) {
            if (cand_costs[idx] == min_load) {
                cand_min_load_edges.push_back(cand_edges[idx]);
            } else {
                break;
            }
        }

        // randomly pick one min-load edge
        size_t rand_pos = Utils::randomUInt(0, cand_min_load_edges.size() - 1, random_generator);
        size_t min_load_edge_id = cand_min_load_edges[rand_pos];
        sol_edges.push_back(min_load_edge_id); // add edge id

        // update load on both vertex
        Edge &min_load_edge = edges_map[min_load_edge_id];
        Vertex &lvtx = vertices_map[lvtx_id];
        Vertex &rvtx = vertices_map[min_load_edge.rvtx->id];
        lvtx.out_degree += min_load_edge.weight;
        lvtx.costs += min_load_edge.cost;
        rvtx.in_degree += min_load_edge.weight;
        rvtx.costs += min_load_edge.cost;


        // if the block is a data / parity block, mark the node as relocated
        if (block_meta.type == DATA_BLK || block_meta.type == PARITY_BLK) {
            Edge &min_load_edge = edges_map[min_load_edge_id];
            size_t min_load_node_id = vtx_to_node_map[min_load_edge.rvtx->id];
            sg_relocated_nodes[min_load_node_id] = true;
        }
    }

    // int sum_costs = 0;
    // for (auto edge_id : sol_edges) {
    //     Edge &edge = edges_map[edge_id];
    //     sum_costs += edge.cost;

    //     printf("edge_id: %ld, cost: %d\n", edge.id, edge.cost);
    // }
    // printf("\n\nnum_edges: %ld, costs: %d\n\n\n\n", sol_edges.size(), sum_costs);

    return true;
}


bool RecvBipartite::findEdgesWithApproachesDP(StripeBatch &stripe_batch, vector<size_t> &sol_edges) {
    // follow the structure of scheduling jobs for m-processor problem in JACM'1976

    ClusterSettings &settings = stripe_batch.getClusterSettings();
    size_t num_nodes = settings.M;

    // initialize the solution edges
    sol_edges.clear();
    
    return true;
}


bool RecvBipartite::constructPartialSolutionFromEdges(StripeBatch &stripe_batch, vector<size_t> &sol_edges, vector<vector<size_t> > &partial_solutions) {
    /** construct partial solutions from recv graph
     * Currently, we have no information about send load distribution; 
     * thus, we first construct partial solutions from recv graph with edges, set from_node to invalid;
     * In send graph, we finalize the send distribution
    **/

    for (auto edge_id : sol_edges) {
        Edge &edge = edges_map[edge_id];
        BlockMeta &block_meta = block_metastore[vtx_to_block_meta_map[edge.lvtx->id]];
        size_t node_id = vtx_to_node_map[edge.rvtx->id];

        // add partial solution: <sg_id, block_type, stripe_id, block_id, from_node, to_node>
        vector<size_t> partial_solution;
        partial_solution.push_back(block_meta.stripe_group_id);
        partial_solution.push_back(block_meta.type);
        partial_solution.push_back(block_meta.stripe_id);
        partial_solution.push_back(block_meta.block_id);
        partial_solution.push_back(INVALID_ID); // from node temporarily set to invalid
        partial_solution.push_back(node_id);

        partial_solutions.push_back(partial_solution);
    }

    return true;
}

bool RecvBipartite::updatePartialSolutionFromRecvGraph(StripeBatch &stripe_batch, vector<vector<size_t> > &partial_solutions, vector<vector<size_t> > &solutions) {
    // construct solution <sg_id, block_type, stripe_id, block_id, from_node, to_node> from recv graph
    // input: in each of solution from recv graph, we set from_node to invalid, then we make it initialized in send graph
    // output: <stripe_id, block_id, from_node, to_node>

    ConvertibleCode &code = stripe_batch.getCode();
    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    // stripe group parity block compute map
    unordered_map<size_t, vector<size_t> > sg_data_distribution_map;
    unordered_map<size_t, vector<size_t> > sg_parity_comp_node_map;
    unordered_map<size_t, vector<pair<size_t, size_t> > > sg_block_swap_map; // <stripe group id, <from_node, to_node> >
    for (auto &stripe_group : stripe_groups) {
        sg_data_distribution_map[stripe_group.getId()] = stripe_group.getDataDistribution();
        sg_parity_comp_node_map[stripe_group.getId()] = vector<size_t>(code.m_f, INVALID_ID); // for each parity block, mark from_node as invalid
        sg_block_swap_map[stripe_group.getId()].clear();
    }


    // initialize solutions
    solutions.clear();

    for (auto &partial_solution : partial_solutions) {
        // construct solution <sg_id, block_type, stripe_id, block_id, from_node, to_node, load>
        size_t sg_id = partial_solution[0];
        size_t block_type = partial_solution[1];
        size_t stripe_id = partial_solution[2];
        size_t block_id = partial_solution[3];
        size_t from_node_id = partial_solution[4];
        size_t to_node_id = partial_solution[5];

        StripeGroup &stripe_group = stripe_groups[sg_id];

        // get parity block compute
        vector<size_t> &sg_parity_comp_nodes = sg_parity_comp_node_map[sg_id];

        if (block_type == COMPUTE_BLK_RE) {
            // for each data block not stored at to_node_id, create a data block relocation task
            for (size_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
                Stripe &stripe = *stripe_group.getStripes()[stripe_id];
                vector<size_t> &stripe_indices = stripe.getStripeIndices();

                for (size_t db_blk_id = 0; db_blk_id < code.k_i; db_blk_id++) {
                    size_t data_blk_from_node_id = stripe_indices[db_blk_id];
                    if (data_blk_from_node_id != to_node_id) {
                        // add solution
                        vector<size_t> solution;
                        solution.push_back(stripe_id);
                        solution.push_back(db_blk_id);
                        solution.push_back(data_blk_from_node_id);
                        solution.push_back(to_node_id);

                        solutions.push_back(solution);
                    }
                }
            }

            // mark in parity block compute map (parity computation is at to_node_id)
            for (size_t parity_id = 0; parity_id < code.m_f; parity_id++) {
                sg_parity_comp_nodes[parity_id] = to_node_id;
            }

        } else if (block_type == COMPUTE_BLK_PM) {
            // for each parity block parity_id not stored at to_node_id, create a parity block relocation task
            
            size_t parity_id = block_id - code.k_f;
            for (size_t stripe_id = 0; stripe_id < code.lambda_i; stripe_id++) {
                Stripe &stripe = *stripe_group.getStripes()[stripe_id];
                vector<size_t> &stripe_indices = stripe.getStripeIndices();
                size_t parity_blk_from_node_id = stripe_indices[code.k_i + parity_id];

                if (parity_blk_from_node_id != to_node_id) {
                    // add solution
                    vector<size_t> solution;
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
            size_t parity_id = block_id - code.k_f;
            size_t parity_from_node_id = sg_parity_comp_nodes[parity_id];

            if (parity_from_node_id == INVALID_ID) {
                printf("error: invalid from_id %ld for parity_id: %ld\n", parity_from_node_id, parity_id);
            }

            // optimization: if the parity compute node does not store a data block, we can relocate the parity block at the compute node, thus no need to add a relocation task
            // we create a block swap task
            vector<size_t> &data_distribution = sg_data_distribution_map[sg_id];
            if (data_distribution[parity_from_node_id] == 0) {
                vector<pair<size_t, size_t> > &sg_block_swap_tasks = sg_block_swap_map[sg_id];
                sg_block_swap_tasks.push_back(pair<size_t, size_t>(parity_from_node_id, to_node_id));
            } else {
                // add solution
                vector<size_t> solution;
                solution.push_back(stripe_id);
                solution.push_back(block_id);
                solution.push_back(parity_from_node_id);
                solution.push_back(to_node_id);

                solutions.push_back(solution);
            }
        } else if (block_type == DATA_BLK) {
            Stripe &stripe = *stripe_group.getStripes()[stripe_id];
            vector<size_t> &stripe_indices = stripe.getStripeIndices();
            from_node_id = stripe_indices[block_id];

            // first check if there is a block swap task
            vector<pair<size_t, size_t> > &sg_block_swap_tasks = sg_block_swap_map[sg_id];
            for (auto it = sg_block_swap_tasks.begin(); it != sg_block_swap_tasks.end(); it++) {
                // change the to_node_id to from_node_id in block swap task, then remove the block swap task
                size_t task_from_node = it->first;
                size_t task_to_node = it->second;
                if (task_to_node == to_node_id) {
                    to_node_id = task_from_node;
                    sg_block_swap_tasks.erase(it);
                    break;
                }
            }

            // add solution
            vector<size_t> solution;
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