#include "RecvBipartite.hh"

RecvBipartite::RecvBipartite()
{
}

RecvBipartite::~RecvBipartite()
{
}

bool RecvBipartite::addStripeBatch(StripeBatch &stripe_batch) {

    for (auto stripe_group : stripe_batch.getStripeGroups()) {
        if (addStripeGroup(stripe_group) == false) {
            return false;
        }
    }

    return true;
}

bool RecvBipartite::addStripeGroup(StripeGroup &stripe_group) {
    // construct for a single stripe group
    ConvertibleCode &code = stripe_group.getCode();

    // if k' = alpha * k
    if (code.k_f == code.k_i * code.alpha) {
        // add data blocks to bipartite graph
        if (addStripeGroupWithData(stripe_group) == false) {
            return false;
        }
        // add parity merging to bipartite graph
        if (ENABLE_PARITY_MERGING == true) {
            if (addStripeGroupWithParityMerging(stripe_group) == false) {
                return false;
            }
        }

        // add re-encoding to bipartite graph
        if (ENABLE_RE_ENCODING == true) {
            if (addStripeGroupWithReEncoding(stripe_group) == false) {
                return false;
            } 
        }

        // add partial parity merging to bipartite graph
        if (ENABLE_PARTIAL_PARITY_MERGING == true) {
            if (addStripeGroupWithPartialParityMerging(stripe_group) == false) {
                return false;
            }
        }
    } else {
        return false;
    }

    return true;
}

bool RecvBipartite::addStripeGroupWithData(StripeGroup &stripe_group) {
    ConvertibleCode &code = stripe_group.getCode();

    // if k' = alpha * k
    if (code.k_f != code.k_i * code.alpha || code.m_f > code.m_i) {
        printf("invalid parameters\n");
        return false;
    }

    ClusterSettings &cluster_settings = stripe_group.getClusterSettings();
    int num_nodes = cluster_settings.M;

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
    Utils::print_int_vector(num_data_stored);

    // candidate nodes for data relocation
    vector<int> data_relocation_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] == 0) {
            data_relocation_candidates.push_back(node_id);
        }
    }

    printf("data relocation candidates:\n");
    Utils::print_int_vector(data_relocation_candidates);

    // data relocation
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] > 1) {
            bool block_overlapped = false;
            for (int stripe_id = 0; stripe_id < code.alpha; stripe_id++) {
                vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();
                for (int block_id = 0; block_id < code.k_i; block_id++) {
                    // add a relocation edge node for this data block
                    if (stripe_indices[block_id] != node_id) {
                        continue;
                    }

                    if (block_overlapped == false) {
                        // mark it as the first block, and search the next stripe
                        block_overlapped = true;
                        break;
                    }

                    BlockMeta block_meta = {
                        .id = -1,
                        .type = DATA_BLK,
                        .stripe_batch_id = -1,
                        .stripe_group_id = stripe_group.getId(),
                        .stripe_id_global = stripes[stripe_id].getId(),
                        .stripe_id = stripe_id,
                        .block_id = block_id,
                        .vtx_id = -1
                    };
                    Vertex &bvtx = *get_block_vtx(block_meta);


                    // 2. add candidate nodes as right vertices
                    for (auto data_relocation_candidate : data_relocation_candidates) {
                        
                        NodeMeta node_meta = {
                            .id = -1,
                            .node_id = data_relocation_candidate,
                            .vtx_id = -1
                        };
                        Vertex &nvtx = *get_node_vtx(node_meta);

                        // add edge (bvtx -> nvtx)
                        Edge edge = {
                            .id = -1,
                            .lvtx = &bvtx,
                            .rvtx = &nvtx,
                            .weight = 1, // edge has weight = 1
                            .cost = 1  // relocation edge has cost = 1
                        };
                        edge.id = edges_map.size();
                        edges_map[edge.id] = edge;

                        bvtx.out_degree += edge.weight; // increase the out_degree of block vertex
                        nvtx.in_degree += edge.weight; // increase the in_degree of node vertex

                    }
                }
            }
        }
    }

    return true;
}

bool RecvBipartite::addStripeGroupWithParityMerging(StripeGroup &stripe_group) {
    ConvertibleCode &code = stripe_group.getCode();

    // if k' = alpha * k
    if (code.k_f != code.k_i * code.alpha || code.m_f > code.m_i) {
        printf("invalid parameters\n");
        return false;
    }

    ClusterSettings &cluster_settings = stripe_group.getClusterSettings();
    int num_nodes = cluster_settings.M;

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
    Utils::print_int_vector(num_data_stored);

    // candidate nodes for data relocation (no any data block stored on that node)
    vector<int> data_relocation_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] == 0) {
            data_relocation_candidates.push_back(node_id);
        }
    }

    printf("data relocation candidates:\n");
    Utils::print_int_vector(data_relocation_candidates);

    // parity relocation

    // candidate nodes for parity relocation (the same as data relocation)
    vector<int> parity_relocation_candidates = data_relocation_candidates;

    // proceed for every parity block
    for (int parity_id = 0; parity_id < code.m_f; parity_id++) {

        BlockMeta block_meta = {
            .id = -1,
            .type = PARITY_BLK,
            .stripe_batch_id = -1,
            .stripe_group_id = stripe_group.getId(),
            .stripe_id_global = -1,
            .stripe_id = -1,
            .block_id = code.k_f + parity_id,
            .vtx_id = -1
        };
        Vertex &bvtx = *get_block_vtx(block_meta);

        for (auto parity_relocation_candidate : parity_relocation_candidates) {
            // calculate the number of parity blocks with the same parity_id at the candidate node
            int num_parity_blocks_required = code.alpha;
            for (int stripe_id = 0; stripe_id < code.alpha; stripe_id++) {
                vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();
                if (stripe_indices[code.k_i + parity_id] == parity_relocation_candidate) {
                    num_parity_blocks_required -= 1;
                }
            }

            NodeMeta node_meta = {
                .id = -1,
                .node_id = parity_relocation_candidate,
                .vtx_id = -1
            };

            Vertex &nvtx = *get_node_vtx(node_meta);

            // add edge
            Edge edge = {
                .id = -1,
                .lvtx = &bvtx,
                .rvtx = &nvtx,
                .weight = 1,
                .cost = num_parity_blocks_required // cost: number of data blocks needed to repair the parity block
            };
            edge.id = edges_map.size();
            edges_map[edge.id] = edge;
            
            bvtx.out_degree += edge.weight; // increase the out_degree of block vertex
            nvtx.in_degree += edge.weight; // increase the in_degree of node vertex
        }

    }

    return true;
}


bool RecvBipartite::addStripeGroupWithReEncoding(StripeGroup &stripe_group) {

    ConvertibleCode &code = stripe_group.getCode();

    // if k' = alpha * k
    if (code.k_f != code.k_i * code.alpha || code.m_f > code.m_i) {
        printf("invalid parameters\n");
        return false;
    }

    ClusterSettings &cluster_settings = stripe_group.getClusterSettings();
    int num_nodes = cluster_settings.M;

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
    Utils::print_int_vector(num_data_stored);

    // candidate nodes for data relocation (no any data block stored on that node)
    vector<int> data_relocation_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] == 0) {
            data_relocation_candidates.push_back(node_id);
        }
    }

    printf("data relocation candidates:\n");
    Utils::print_int_vector(data_relocation_candidates);

    // parity relocation

    // candidate nodes for parity relocation (the same as data relocation)
    vector<int> parity_relocation_candidates = data_relocation_candidates;

    // Step 1. create a compute block (on left) to represent the dummy block to do re-encoding

    // 1. get vertex of compute block 
    BlockMeta compute_block_meta = {
        .id = -1,
        .type = COMPUTE_BLK,
        .stripe_batch_id = -1,
        .stripe_group_id = stripe_group.getId(),
        .stripe_id_global = -1,
        .stripe_id = -1,
        .block_id = -1,
        .vtx_id = -1
    };
    Vertex &cb_vtx = *get_block_vtx(compute_block_meta);

    // Step 1.2 create n edges to represent the number blocks required to do re-encoding. On a node with x data blocks, the edge is with cost (k_f - x)
    
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        int num_data_blocks_required = code.k_f - num_data_stored[node_id];

        // find vertex of the node
        NodeMeta node_meta = {
            .id = -1,
            .node_id = node_id,
            .vtx_id = -1
        };
        Vertex &nvtx = *get_node_vtx(node_meta);

        // add edge
        Edge edge = {
            .id = -1,
            .lvtx = &cb_vtx,
            .rvtx = &nvtx,
            .weight = 1,
            .cost = num_data_blocks_required // cost: number of data blocks needed to repair the parity block
        };
        edge.id = edges_map.size();
        edges_map[edge.id] = edge;
        
        // increase the out-degree for block
        cb_vtx.out_degree += edge.weight; // increase the out_degree of block vertex
        nvtx.in_degree += edge.weight; // increase the in_degree of node vertex
    }


    // Step 2 for each parity block, create a compute node in the middle of the bipartite graph to represent re-encoding, and connect them

    for (int parity_id = 0; parity_id < code.m_f; parity_id++) {

        // 2.1 get vertex of the parity block 
        BlockMeta parity_block_meta = {
            .id = -1,
            .type = PARITY_BLK,
            .stripe_batch_id = -1,
            .stripe_group_id = stripe_group.getId(),
            .stripe_id_global = -1,
            .stripe_id = -1,
            .block_id = code.k_f + parity_id,
            .vtx_id = -1
        };
        Vertex &bvtx = *get_block_vtx(parity_block_meta);

        // 2.2 get vertex of compute node corresponding to the parity block
        BlockMeta compute_node_meta = {
            .id = -1,
            .type = COMPUTE_NODE,
            .stripe_batch_id = -1,
            .stripe_group_id = stripe_group.getId(),
            .stripe_id_global = -1,
            .stripe_id = -1,
            .block_id = -1,
            .vtx_id = -1
        };
        Vertex &cn_vtx = *get_block_vtx(compute_node_meta);

        // 2.3 connect the parity block and compute node
        // add edge
        Edge edge = {
            .id = -1,
            .lvtx = &bvtx,
            .rvtx = &cn_vtx,
            .weight = 1,
            .cost = 0 // cost: no cost from parity block to compute block
        };
        edge.id = edges_map.size();
        edges_map[edge.id] = edge;

        // 2.4 for each parity block relocation candidate, connect them with compute node

        for (auto parity_relocation_candidate : parity_relocation_candidates) {
            // find vertex of the node
            NodeMeta node_meta = {
                .id = -1,
                .node_id = parity_relocation_candidate,
                .vtx_id = -1
            };
            Vertex &nvtx = *get_node_vtx(node_meta);

            // add edge
            Edge edge = {
                .id = -1,
                .lvtx = &cn_vtx,
                .rvtx = &nvtx,
                .weight = 1,
                .cost = 1 // cost: 1 (from compute node to parity node representing relocation)
            };
            edge.id = edges_map.size();
            edges_map[edge.id] = edge;
            
            cn_vtx.out_degree += edge.weight; // increase the out_degree of block vertex
            nvtx.in_degree += edge.weight; // increase the in_degree of node vertex
        }
    }

    return true;
}

bool RecvBipartite::addStripeGroupWithPartialParityMerging(StripeGroup &stripe_group) {
    return true;
}

void RecvBipartite::print_meta() {
    printf("block_meta:\n");

    printf("data_block_meta:\n");
    for (auto it = data_block_meta_map.begin(); it != data_block_meta_map.end(); it++) {
        BlockMeta &block_meta = it->second;
        printf("id: %d, type: %d, vtx_id: %d, stripe_batch_id: %d, stripe_group_id: %d, stripe_id: %d, stripe_id (global): %d, block_id: %d\n", block_meta.id, block_meta.type, block_meta.vtx_id, block_meta.stripe_batch_id, block_meta.stripe_group_id, block_meta.stripe_id, block_meta.stripe_id_global, block_meta.block_id);
    }

    printf("parity_block_meta:\n");
    for (auto it = parity_block_meta_map.begin(); it != parity_block_meta_map.end(); it++) {
        BlockMeta &block_meta = it->second;
        printf("id: %d, type: %d, vtx_id: %d, stripe_batch_id: %d, stripe_group_id: %d, stripe_id: %d, stripe_id (global): %d, block_id: %d\n", block_meta.id, block_meta.type, block_meta.vtx_id, block_meta.stripe_batch_id, block_meta.stripe_group_id, block_meta.stripe_id, block_meta.stripe_id_global, block_meta.block_id);
    }

    printf("compute_block_meta:\n");
    for (auto it = compute_block_meta_map.begin(); it != compute_block_meta_map.end(); it++) {
        BlockMeta &block_meta = it->second;
        printf("id: %d, type: %d, vtx_id: %d, stripe_batch_id: %d, stripe_group_id: %d, stripe_id: %d, stripe_id (global): %d, block_id: %d\n", block_meta.id, block_meta.type, block_meta.vtx_id, block_meta.stripe_batch_id, block_meta.stripe_group_id, block_meta.stripe_id, block_meta.stripe_id_global, block_meta.block_id);
    }

    printf("compute_node_meta:\n");
    for (auto it = compute_node_meta_map.begin(); it != compute_node_meta_map.end(); it++) {
        BlockMeta &block_meta = it->second;
        printf("id: %d, type: %d, vtx_id: %d, stripe_batch_id: %d, stripe_group_id: %d, stripe_id: %d, stripe_id (global): %d, block_id: %d\n", block_meta.id, block_meta.type, block_meta.vtx_id, block_meta.stripe_batch_id, block_meta.stripe_group_id, block_meta.stripe_id, block_meta.stripe_id_global, block_meta.block_id);
    }

    printf("node_meta:\n");
    for (auto it = node_meta_map.begin(); it != node_meta_map.end(); it++) {
        NodeMeta &node_meta = it->second;
        printf("id: %d, node_id: %d, vtx_id: %d\n", node_meta.id, node_meta.node_id, node_meta.vtx_id);
    }
}

void RecvBipartite::print() {
    printf("recv bipartite graph:\n");
    printf("Left vertices:\n");
    for (auto it = left_vertices_map.begin(); it != left_vertices_map.end(); it++) {
        Vertex &lvtx = *(it->second);
        printf("id: %d, in_degree: %d, out_degree: %d, weights: %d, costs: %d\n", lvtx.id, lvtx.in_degree, lvtx.out_degree, lvtx.weights, lvtx.costs);
    }

    printf("Right vertices:\n");
    for (auto it = right_vertices_map.begin(); it != right_vertices_map.end(); it++) {
        Vertex &rvtx = *(it->second);
        printf("id: %d, in_degree: %d, out_degree: %d, weights: %d, costs: %d\n", rvtx.id, rvtx.in_degree, rvtx.out_degree, rvtx.weights, rvtx.costs);
    }

    printf("Internal vertices:\n");
    for (auto it = internal_vertices_map.begin(); it != internal_vertices_map.end(); it++) {
        Vertex &ivtx = *(it->second);
        printf("id: %d, in_degree: %d, out_degree: %d, weights: %d, costs: %d\n", ivtx.id, ivtx.in_degree, ivtx.out_degree, ivtx.weights, ivtx.costs);
    }

    printf("Edges: \n");
    for (auto it = edges_map.begin(); it != edges_map.end(); it++) {
        int edge_id = it->first;
        Edge &edge = it->second;
        printf("id: %d, lvtx(.id): %d, rvtx(.node_id): %d, weight: %d, cost: %d\n", edge_id, edge.lvtx->id, edge.rvtx->id, edge.weight, edge.cost);
    }
}

Vertex *RecvBipartite::get_block_vtx(BlockMeta &in_block_meta) {

    // 1. get block map
    map<int, BlockMeta> *block_map_ptr = NULL;

    if (in_block_meta.type == DATA_BLK) {
        block_map_ptr = &data_block_meta_map;
    } else if (in_block_meta.type == PARITY_BLK) {
        block_map_ptr = &parity_block_meta_map;
    } else if (in_block_meta.type == COMPUTE_BLK) {
        block_map_ptr = &compute_block_meta_map;
    } else if (in_block_meta.type == COMPUTE_NODE) {
        block_map_ptr = &compute_node_meta_map;
    }

    if (block_map_ptr == NULL) {
        printf("invalid block metadata\n");
        return NULL;
    }

    // 2. get vertex of data block 
    int bvtx_id = -1;
    map<int, BlockMeta> &block_map = *block_map_ptr;

    for (auto it = block_map.begin(); it != block_map.end(); it++) {
        // find vertex id from block metadata
        BlockMeta &block_meta = it->second;
        if (
            block_meta.type == in_block_meta.type &&
            block_meta.stripe_batch_id == in_block_meta.stripe_batch_id &&
            block_meta.stripe_group_id == in_block_meta.stripe_group_id && 
            block_meta.stripe_id == in_block_meta.stripe_id && 
            block_meta.stripe_id_global == in_block_meta.stripe_id_global && 
            block_meta.block_id == in_block_meta.block_id
        ) {
            bvtx_id = block_meta.vtx_id;
            break;
        }
    }

    // block metadata not found
    if (bvtx_id == -1) {
        // add block as left vertex
        Vertex bvtx = {
            .id = -1,
            .in_degree = 0,
            .out_degree = 0,
            .weights = 0,
            .costs = 0
        };

        bvtx.id = vertices_map.size();
        bvtx_id = bvtx.id;
        vertices_map[bvtx_id] = bvtx;
        // left vertex
        if (in_block_meta.type == DATA_BLK || in_block_meta.type == PARITY_BLK || in_block_meta.type == COMPUTE_BLK) {
            left_vertices_map[bvtx_id] = &vertices_map[bvtx_id];
        } else if (in_block_meta.type == COMPUTE_NODE) {
            internal_vertices_map[bvtx_id] = &vertices_map[bvtx_id];
        }

        // add block metadata
        BlockMeta new_block_meta = in_block_meta;
        
        new_block_meta.id = block_map.size();
        block_map[new_block_meta.id] = new_block_meta;
    }

    if (in_block_meta.type == DATA_BLK || in_block_meta.type == PARITY_BLK || in_block_meta.type == COMPUTE_BLK) {
        return left_vertices_map[bvtx_id];
    } else if (in_block_meta.type == COMPUTE_NODE) {
        return internal_vertices_map[bvtx_id];
    } else {
        return NULL;
    }
}

Vertex *RecvBipartite::get_node_vtx(NodeMeta &in_node_meta) {
    // find vertex of the node
    int nvtx_id = -1;
    // check from node meta
    for (auto it = node_meta_map.begin(); it != node_meta_map.end(); it++) {
        NodeMeta &node_meta = it->second;
        if (node_meta.node_id == in_node_meta.node_id) {
            nvtx_id = node_meta.vtx_id;
            break;
        }
    }

    // add vertex and metadata
    if (nvtx_id == -1) {
        Vertex nvtx = {
            .id = -1,
            .in_degree = 0,
            .out_degree = 0,
            .weights = 0,
            .costs = 0
        };
        nvtx.id = vertices_map.size();
        nvtx_id = nvtx.id;
        vertices_map[nvtx_id] = nvtx;
        right_vertices_map[nvtx_id] = &vertices_map[nvtx_id];
        
        NodeMeta node_meta = {
            .id = -1,
            .node_id = in_node_meta.node_id,
            .vtx_id = nvtx_id
        };

        node_meta.id = node_meta_map.size();
        node_meta_map[node_meta.id] = node_meta;
    }

    return right_vertices_map[nvtx_id];
}