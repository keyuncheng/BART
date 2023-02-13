#include "SendBipartite.hh"

SendBipartite::SendBipartite(/* args */)
{
}

SendBipartite::~SendBipartite()
{
}

bool SendBipartite::addStripeBatchFromRecvGraph(StripeBatch &stripe_batch,  RecvBipartite &recv_bipartite) {
    for (auto stripe_group : stripe_batch.getStripeGroups()) {
        if (addStripeGroupFromRecvGraph(stripe_group, recv_bipartite) == false) {
            return false;
        }
    }

    return true;
}

bool SendBipartite::addStripeGroupFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite) {
    ConvertibleCode &code = stripe_group.getCode();

    // if k' = alpha * k
    if (code.k_f == code.k_i * code.alpha) {
        // add data blocks to bipartite graph
        if (addStripeGroupWithDataFromRecvGraph(stripe_group, recv_bipartite) == false) {
            return false;
        }
        // add parity merging to bipartite graph
        if (ENABLE_PARITY_MERGING == true) {
            if (addStripeGroupWithParityMergingFromRecvGraph(stripe_group, recv_bipartite) == false) {
                return false;
            }
        }

        // add re-encoding to bipartite graph
        if (ENABLE_RE_ENCODING == true) {
            if (addStripeGroupWithReEncodingFromRecvGraph(stripe_group, recv_bipartite) == false) {
                return false;
            } 
        }

        // add partial parity merging to bipartite graph
        if (ENABLE_PARTIAL_PARITY_MERGING == true) {
            if (addStripeGroupWithPartialParityMergingFromRecvGraph(stripe_group, recv_bipartite) == false) {
                return false;
            }
        }
    } else {
        return false;
    }

    return true;
}

bool SendBipartite::addStripeGroupWithDataFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite) {
    //     ConvertibleCode &code = stripe_group.getCode();

    // // if k' = alpha * k
    // if (code.k_f != code.k_i * code.alpha || code.m_f > code.m_i) {
    //     printf("invalid parameters\n");
    //     return false;
    // }

    // // find data blocks for corresponding stripe group
    // for (auto it = recv_bipartite.left_vertices_map.begin(); it != recv_bipartite.left_vertices_map.end(); it++) {
    //     BlockVtx &bvtx_recv = it->second;
    //     if (bvtx_recv.stripe_group_id != stripe_group.getId()) {
    //         continue;
    //     }

    //     // the receive graph should already have one to one correspondence
    //     if (bvtx_recv.block_id < code.k_i) {
    //         // add data block
    //         BlockVtx bvtx_send = bvtx_recv;
    //         bvtx_send.id = left_vertices_map.size(); // reinitialize the id
    //         bvtx_send.out_degree = 1; // one to one mapping
    //         left_vertices_map[bvtx_send.id] = bvtx_send;

    //         // add node vertex
    //         Stripe &stripe = stripe_group.getStripes()[bvtx_send.stripe_id];
    //         vector<int> stripe_indices = stripe.getStripeIndices();
    //         int recv_node_id = stripe_indices[bvtx_send.block_id];
    //         if (right_vertices_map.find(recv_node_id) == right_vertices_map.end()) {
    //             NodeVtx nvtx_send = {
    //                 .id = -1,
    //                 .in_degree = 1,
    //                 .node_id = recv_node_id,
    //                 .out_degree = 0
    //             };
    //             nvtx_send.id = right_vertices_map.size();
    //             right_vertices_map[recv_node_id] = nvtx_send;
    //         } else {
    //             NodeVtx &nvtx_send = right_vertices_map[recv_node_id];
    //             nvtx_send.in_degree += 1;
    //         }

    //         // add edge
    //         Edge edge = {
    //             .id = -1,
    //             .lvtx = &left_vertices_map[bvtx_send.id],
    //             .rvtx = &right_vertices_map[recv_node_id],
    //             .weight = 1
    //         };
    //         edge.id = edges_map.size();
    //         edges_map[edge.id] = edge;
    //     } else {
    //         // add parity block
    //         for (int parity_id = 0; parity_id < code.m_f; parity_id++) {
    //             int recv_node_id = -1;
    //             int recv_edge_weight = -1;
    //             // identify the location of the parity block that will be stored
    //             for (auto it = recv_bipartite.edges_map.begin(); it != recv_bipartite.edges_map.end(); it++) {
    //                 Edge &edge = it->second;
    //                 if (edge.lvtx->id == bvtx_recv.id) {
    //                     recv_node_id = edge.rvtx->node_id;
    //                     recv_edge_weight = edge.weight;
    //                     break;
    //                 }
    //             }

    //             int sum_parity_blocks = 0;
    //             for (int stripe_id = 0; stripe_id < code.alpha; stripe_id++) {
    //                 Stripe &stripe = stripe_group.getStripes()[stripe_id];
    //                 vector<int> stripe_indices = stripe.getStripeIndices();
    //                 // need to transfer to recv_node_id
    //                 int send_node_id = stripe_indices[code.k_i + parity_id];
    //                 if (send_node_id != recv_node_id) {
    //                     // add parity block
    //                     BlockVtx bvtx_send = {
    //                         .block_id = code.k_i + parity_id,
    //                         .id = -1,
    //                         .in_degree = 0,
    //                         .out_degree = 1,
    //                         .stripe_batch_id = bvtx_recv.stripe_batch_id,
    //                         .stripe_group_id = bvtx_recv.stripe_group_id,
    //                         .stripe_id = stripe_id,
    //                         .stripe_id_global = stripe.getId()
    //                     };
    //                     bvtx_send.id = left_vertices_map.size();
    //                     left_vertices_map[bvtx_send.id] = bvtx_send;

    //                     // add node id
    //                     if (right_vertices_map.find(send_node_id) == right_vertices_map.end()) {
    //                         NodeVtx nvtx_send = {
    //                             .id = -1,
    //                             .in_degree = 1,
    //                             .node_id = send_node_id,
    //                             .out_degree = 0
    //                         };
    //                         nvtx_send.id = right_vertices_map.size();
    //                         right_vertices_map[send_node_id] = nvtx_send;
    //                     } else {
    //                         NodeVtx &nvtx_send = right_vertices_map[send_node_id];
    //                         nvtx_send.in_degree += 1;
    //                     }

    //                     // add edge
    //                     Edge edge = {
    //                         .id = -1,
    //                         .lvtx = &left_vertices_map[bvtx_send.id],
    //                         .rvtx = &right_vertices_map[send_node_id],
    //                         .weight = 1
    //                     };
    //                     edge.id = edges_map.size();
    //                     edges_map[edge.id] = edge;

    //                     sum_parity_blocks += 1;
    //                 }
    //             }

    //             if (sum_parity_blocks != recv_edge_weight) {
    //                 printf("invalid number of parity blocks to be sent!\n");
    //                 return false;
    //             }
    //         }
    //     }
    // }

    return true;
}

bool SendBipartite::addStripeGroupWithParityMergingFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite) {
    return true;
}

bool SendBipartite::addStripeGroupWithReEncodingFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite) {
    return true;
}

bool SendBipartite::addStripeGroupWithPartialParityMergingFromRecvGraph(StripeGroup &stripe_group, RecvBipartite &recv_bipartite) {
    return true;
}

void SendBipartite::print() {
    // printf("recv bipartite graph:\n");
    // printf("Left vertices:\n");
    // for (auto it = left_vertices_map.begin(); it != left_vertices_map.end(); it++) {
    //     int lvtx_id = it->first;
    //     BlockVtx &lvtx = it->second;
    //     printf("id: %d, stripe_group_id: %d, stripe_id_global: %d, block_id: %d, out_degree: %d\n", lvtx_id, lvtx.stripe_group_id, lvtx.stripe_id_global, lvtx.block_id, lvtx.out_degree);
    // }

    // printf("Right vertices:\n");
    // for (auto it = right_vertices_map.begin(); it != right_vertices_map.end(); it++) {
    //     int rvtx_id = it->first;
    //     NodeVtx &rvtx = it->second;
    //     printf("node_id: %d, id: %d, in_degree: %d\n", rvtx_id, rvtx.id, rvtx.in_degree);
    // }

    // printf("Edges: \n");
    // for (auto it = edges_map.begin(); it != edges_map.end(); it++) {
    //     int edge_id = it->first;
    //     Edge &edge = it->second;
    //     printf("id: %d, lvtx(.id): %d, rvtx(.node_id): %d, weight: %d\n", edge_id, edge.lvtx->id, edge.rvtx->node_id, edge.weight);
    // }
}

bool SendBipartite::constructSolutionFromRecvGraph(StripeBatch &stripe_batch, vector<vector<int> > &solutions_from_recv_graph, vector<vector<int> > &solutions) {
    // construct solution <sg_id, block_type, stripe_id, block_id, from_node, to_node> from recv graph
    // input: in each of solution from recv graph, we set from_node to -1, then we make it initialized in send graph
    // output: <stripe_id, block_id, from_node, to_node>

    ConvertibleCode &code = stripe_batch.getCode();
    vector<StripeGroup> &stripe_groups = stripe_batch.getStripeGroups();

    // initialize solutions
    solutions.clear();

    for (auto &partial_solution : solutions_from_recv_graph) {
        int sg_id = partial_solution[0];
        int block_type = partial_solution[1];
        int stripe_id = partial_solution[2];
        int block_id = partial_solution[3];
        int from_node_id = partial_solution[4];
        int to_node_id = partial_solution[5];

        StripeGroup &stripe_group = stripe_groups[sg_id];

        if (block_type == DATA_BLK) {
            Stripe &stripe = *stripe_group.getStripes()[stripe_id];
            vector<int> &stripe_indices = stripe.getStripeIndices();
            from_node_id = stripe_indices[block_id];

            vector<int> solution;
            solution.push_back(stripe_id);
            solution.push_back(block_id);
            solution.push_back(from_node_id);
            solution.push_back(to_node_id);

            solutions.push_back(solution);

        } else if (block_type == PARITY_BLK) {
            int parity_id = block_id - code.k_f;
            vector<Stripe *> &stripes = stripe_group.getStripes();
            for (int sid = 0; sid < code.lambda_i; sid++) {
                vector<int> &stripe_indices = stripes[sid]->getStripeIndices();
                if (stripe_indices[code.k_i + parity_id] != to_node_id) {
                    stripe_id = sid;
                    block_id = code.k_i + parity_id;
                    from_node_id = stripe_indices[code.k_i + parity_id];

                    vector<int> solution;
                    solution.push_back(stripe_id);
                    solution.push_back(block_id);
                    solution.push_back(from_node_id);
                    solution.push_back(to_node_id);

                    solutions.push_back(solution);
                }
            }
        } else if (block_type == COMPUTE_BLK) {
            vector<Stripe *> &stripes = stripe_group.getStripes();
            for (int sid = 0; sid < code.lambda_i; sid++) {
                vector<int> &stripe_indices = stripes[sid]->getStripeIndices();
                for (int bid = 0; bid < code.k_i; bid++) {
                    if (stripe_indices[bid] != to_node_id) {
                        stripe_id = sid;
                        block_id = bid;
                        from_node_id = stripe_indices[bid];

                        vector<int> solution;
                        solution.push_back(stripe_id);
                        solution.push_back(block_id);
                        solution.push_back(from_node_id);
                        solution.push_back(to_node_id);

                        solutions.push_back(solution);
                    }
                }
            }
        } else {
            printf("error: invalid block type\n");
            return false;
        }
    }

    return true;
}