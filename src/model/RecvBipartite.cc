#include "RecvBipartite.hh"

RecvBipartite::RecvBipartite()
{
}

RecvBipartite::~RecvBipartite()
{
}

bool RecvBipartite::addStripeBatch(StripeBatch &stripe_batch) {
    for (auto stripe_group : stripe_batch.getStripeGroups()) {
        addStripeGroup(stripe_group);
    }

    return true;
}

bool RecvBipartite::addStripeGroup(StripeGroup &stripe_group) {
    // construct for a single stripe group
    ConvertibleCode &code = stripe_group.getCode();
    
    bool ret_val = true;

    // if k' = alpha * k
    if (code.k_f == code.k_i * code.alpha) {
        ret_val = addStripeGroupWithParityMerging(stripe_group);
    } else {
        ret_val = false;
    }

    return ret_val;
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

    // candidate nodes for data relocation
    vector<int> data_relocation_candidates;
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] == 0) {
            data_relocation_candidates.push_back(node_id);
        }
    }

    // data relocation
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        if (num_data_stored[node_id] > 1) {
            for (int stripe_id = 1; stripe_id < code.alpha; stripe_id++) {
                vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();
                for (int block_id = 0; block_id < code.n_i; block_id++) {
                    // add a relocation edge node for this data block
                    if (stripe_indices[block_id] == node_id) {
                        // add block as left vertex
                        BlockVtx bvtx = {
                            .id = -1,
                            .block_id = block_id,
                            .in_degree = 0,
                            .out_degree = 1,
                            .stripe_batch_id = -1,
                            .stripe_group_id = stripe_group.getId(),
                            .stripe_id = stripe_id,
                            .stripe_id_global = stripes[stripe_id].getId()
                        };
                        bvtx.id = left_vertices_map.size();
                        left_vertices_map[bvtx.id] = bvtx;

                        // add candidate nodes as right vertices
                        for (auto data_relocation_candidate : data_relocation_candidates) {
                            // check if node id has been added
                            if (right_vertices_map.find(data_relocation_candidate) == right_vertices_map.end()) {
                                NodeVtx nvtx = {
                                    .id = -1,
                                    .in_degree = 1,
                                    .out_degree = 0,
                                    .node_id = data_relocation_candidate
                                };
                                nvtx.id = right_vertices_map.size();
                                right_vertices_map[nvtx.node_id] = nvtx;
                            } else {
                                // increase the in_degree
                                NodeVtx &nvtx = right_vertices_map[data_relocation_candidate];
                                nvtx.in_degree += 1;
                            }

                        }

                        // add edges
                        for (auto data_relocation_candidate : data_relocation_candidates) {
                            Edge edge = {
                                .id = -1,
                                .lvtx = &bvtx,
                                .rvtx = &right_vertices_map[data_relocation_candidate],
                                .weight = 1
                            };
                            edge.id = edges_map.size();
                        }

                    }
                }
            }
        }
    }


    // parity relocation

    // candidate nodes for parity relocation (the same as data relocation)
    vector<int> parity_relocation_candidates = data_relocation_candidates;

    // proceed for every parity block
    for (int parity_id = 0; parity_id < code.m_f; parity_id++) {
        // add parity block
        BlockVtx bvtx = {
            .id = -1,
            .block_id = code.k_f + parity_id,
            .stripe_id = -1,
            .stripe_id_global = -1,
            .stripe_group_id = stripe_group.getId(),
            .stripe_batch_id = -1,
            .in_degree = 0,
            .out_degree = 0,
        };
        bvtx.id = left_vertices_map.size();
        for (auto parity_relocation_candidate : parity_relocation_candidates) {
            // calculate the number of parity blocks with the same parity_id at the candidate node
            int num_parity_blocks = 0;
            for (int stripe_id = 0; stripe_id < code.alpha; stripe_id++) {
                vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();
                if (stripe_indices[code.k_i + parity_id] == parity_relocation_candidate) {
                    num_parity_blocks += 1;
                }
            }

            // add node vertex
            if (right_vertices_map.find(parity_relocation_candidate) == right_vertices_map.end()) {
                NodeVtx nvtx = {
                    .id = -1,
                    .in_degree = num_parity_blocks,
                    .out_degree = 0,
                    .node_id = parity_relocation_candidate
                };
                nvtx.id = right_vertices_map.size();
                right_vertices_map[nvtx.node_id] = nvtx;
            } else {
                // increase the in_degree
                NodeVtx &nvtx = right_vertices_map[parity_relocation_candidate];
                nvtx.in_degree += num_parity_blocks;
            }

            // add edge
            Edge edge = {
                .id = -1,
                .lvtx = &bvtx,
                .rvtx = &right_vertices_map[parity_relocation_candidate],
                .weight = num_parity_blocks
            };
            edge.id = edges_map.size();
        }
    }

    return true;
}