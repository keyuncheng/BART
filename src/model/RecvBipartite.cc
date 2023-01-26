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

                    // 1. get BlockVtx
                    int bvtx_id = -1;
                    for (auto it = left_vertices_map.begin(); it != left_vertices_map.end(); it++) {
                        BlockVtx &bvtx = it->second;
                        if (
                            bvtx.stripe_group_id == stripe_group.getId() && 
                            bvtx.stripe_id == stripe_id && 
                            bvtx.stripe_id_global == stripes[stripe_id].getId() && 
                            bvtx.block_id == block_id
                        ) {
                            bvtx_id = bvtx.id;
                        } else {
                            // add block as left vertex
                            BlockVtx bvtx = {
                                .id = -1,
                                .stripe_batch_id = -1,
                                .stripe_group_id = stripe_group.getId(),
                                .stripe_id_global = stripes[stripe_id].getId(),
                                .stripe_id = stripe_id,
                                .block_id = block_id,
                                .in_degree = 0,
                                .out_degree = 1,
                                .costs = 0
                            };
                            bvtx.id = left_vertices_map.size();
                            left_vertices_map[bvtx.id] = bvtx;
                            bvtx_id = bvtx.id;
                        }
                    }

                    BlockVtx &bvtx = left_vertices_map[bvtx_id];


                    // 2. add candidate nodes as right vertices
                    for (auto data_relocation_candidate : data_relocation_candidates) {
                        // check if node has been added before
                        if (right_vertices_map.find(data_relocation_candidate) == right_vertices_map.end()) {
                            NodeVtx nvtx = {
                                .id = -1,
                                .node_id = data_relocation_candidate,
                                .in_degree = 1,
                                .out_degree = 0,
                                .costs = 0
                            };
                            nvtx.id = right_vertices_map.size();
                            right_vertices_map[nvtx.node_id] = nvtx;
                        } else {
                            // increase the in_degree
                            NodeVtx &nvtx = right_vertices_map[data_relocation_candidate];
                            nvtx.in_degree += 1;
                        }

                        // add corresponding edge
                        BNEdge edge = {
                            .id = -1,
                            .lvtx = &left_vertices_map[bvtx.id],
                            .rvtx = &right_vertices_map[data_relocation_candidate],
                            .weight = 1, // edges have weight = 1
                            .cost = 1  // relocating a data block have cost = 1
                        };
                        edge.id = bnedges_map.size();
                        bnedges_map[edge.id] = edge;

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

        // 1. get BlockVtx
        int bvtx_id = -1;
        for (auto it = left_vertices_map.begin(); it != left_vertices_map.end(); it++) {
            BlockVtx &bvtx = it->second;
            if (
                bvtx.stripe_group_id == stripe_group.getId() && 
                bvtx.block_id == code.k_f + parity_id
            ) {
                bvtx_id = bvtx.id;
            } else {
                // add parity block
                BlockVtx bvtx = {
                    .id = -1,
                    .stripe_batch_id = -1,
                    .stripe_group_id = stripe_group.getId(),
                    .stripe_id = -1,
                    .stripe_id_global = -1,
                    .block_id = code.k_f + parity_id,
                    .in_degree = 0,
                    .out_degree = 0,
                    .costs = 0
                };
                bvtx.id = left_vertices_map.size();
                left_vertices_map[bvtx.id] = bvtx;
                bvtx_id = bvtx.id;
            }
        }

        BlockVtx &bvtx = left_vertices_map[bvtx_id];

        for (auto parity_relocation_candidate : parity_relocation_candidates) {
            // calculate the number of parity blocks with the same parity_id at the candidate node
            int num_parity_blocks_required = code.alpha;
            for (int stripe_id = 0; stripe_id < code.alpha; stripe_id++) {
                vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();
                if (stripe_indices[code.k_i + parity_id] == parity_relocation_candidate) {
                    num_parity_blocks_required -= 1;
                }
            }

            // add node vertex
            if (right_vertices_map.find(parity_relocation_candidate) == right_vertices_map.end()) {
                NodeVtx nvtx = {
                    .id = -1,
                    .node_id = parity_relocation_candidate,
                    .in_degree = num_parity_blocks_required,
                    .out_degree = 0,
                    .costs = 0
                };
                nvtx.id = right_vertices_map.size();
                right_vertices_map[nvtx.node_id] = nvtx;
            } else {
                // increase the in_degree
                NodeVtx &nvtx = right_vertices_map[parity_relocation_candidate];
                nvtx.in_degree += num_parity_blocks_required;
            }

            // add edge
            BNEdge edge = {
                .id = -1,
                .lvtx = &left_vertices_map[bvtx.id],
                .rvtx = &right_vertices_map[parity_relocation_candidate],
                .weight = 1,
                .cost = num_parity_blocks_required // cost: number of data blocks needed to repair the parity block
            };
            edge.id = bnedges_map.size();
            bnedges_map[edge.id] = edge;
            
            // increase the out-degree for block
            bvtx.out_degree += num_parity_blocks_required;
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

    // Step 1. create a compute vertex to represent the dummy block to do re-encoding

    // Step 1.1 get ComputeVtx
    int cvtx_id = -1;
    for (auto it = compute_vtx_map.begin(); it != compute_vtx_map.end(); it++) {
        ComputeVtx &cvtx = it->second;
        if (
            cvtx.stripe_group_id == stripe_group.getId()
        ) {
            cvtx_id = cvtx.id;
        } else {
            // add parity block
            ComputeVtx cvtx = {
                .id = -1,
                .stripe_batch_id = -1,
                .stripe_group_id = stripe_group.getId(),
                .stripe_id = -1,
                .stripe_id_global = -1,
                .block_id = -1,
                .in_degree = 0,
                .out_degree = 0,
                .costs = 0
            };
            cvtx.id = compute_vtx_map.size();
            compute_vtx_map[cvtx.id] = cvtx;
            cvtx_id = cvtx.id;
        }
    }

    ComputeVtx &bvtx = compute_vtx_map[cvtx_id];

    // Step 1.2 create n edges to represent the number blocks required to do re-encoding. On a node with x data blocks, the edge is with cost (k_f - x)
    
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        int num_data_blocks_required = code.k_f - num_data_stored[node_id];

        // add node vertex
        if (right_vertices_map.find(node_id) == right_vertices_map.end()) {
            NodeVtx nvtx = {
                .id = -1,
                .node_id = node_id,
                .in_degree = num_data_blocks_required,
                .out_degree = 0,
                .costs = 0
            };
            nvtx.id = right_vertices_map.size();
            right_vertices_map[nvtx.node_id] = nvtx;
        } else {
            // increase the in_degree
            NodeVtx &nvtx = right_vertices_map[node_id];
            nvtx.in_degree += num_data_blocks_required;
        }

        // add edge
        BCEdge edge = {
            .id = -1,
            .lvtx = &left_vertices_map[bvtx.id],
            .rvtx = &right_vertices_map[parity_relocation_candidate],
            .weight = 1,
            .cost = num_parity_blocks_required // cost: number of data blocks needed to repair the parity block
        };
        edge.id = edges_map.size();
        edges_map[edge.id] = edge;
        
        // increase the out-degree for block
        bvtx.out_degree += num_parity_blocks_required;
    }

    return true;
}

bool RecvBipartite::addStripeGroupWithPartialParityMerging(StripeGroup &stripe_group) {
    return true;
}


void RecvBipartite::print() {
    printf("recv bipartite graph:\n");
    printf("Left vertices:\n");
    for (auto it = left_vertices_map.begin(); it != left_vertices_map.end(); it++) {
        int lvtx_id = it->first;
        BlockVtx &lvtx = it->second;
        printf("id: %d, stripe_group_id: %d, stripe_id_global: %d, block_id: %d, out_degree: %d\n", lvtx_id, lvtx.stripe_group_id, lvtx.stripe_id_global, lvtx.block_id, lvtx.out_degree);
    }

    printf("Right vertices:\n");
    for (auto it = right_vertices_map.begin(); it != right_vertices_map.end(); it++) {
        int rvtx_id = it->first;
        NodeVtx &rvtx = it->second;
        printf("node_id: %d, id: %d, in_degree: %d\n", rvtx_id, rvtx.id, rvtx.in_degree);
    }

    printf("Edges: \n");
    for (auto it = edges_map.begin(); it != edges_map.end(); it++) {
        int edge_id = it->first;
        Edge &edge = it->second;
        printf("id: %d, lvtx(.id): %d, rvtx(.node_id): %d, weight: %d\n", edge_id, edge.lvtx->id, edge.rvtx->node_id, edge.weight);
    }
}