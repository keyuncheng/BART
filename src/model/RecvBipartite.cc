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

                    // get data block vertex
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
                    Vertex &bvtx = *getBlockVtx(block_meta);


                    // 2. add candidate nodes for data relocation as node vertices
                    for (auto data_relocation_candidate : data_relocation_candidates) {
                        
                        NodeMeta node_meta = {
                            .id = -1,
                            .node_id = data_relocation_candidate,
                            .vtx_id = -1
                        };
                        Vertex &nvtx = *getNodeVtx(node_meta);

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

    // relocate for every parity block
    for (int parity_id = 0; parity_id < code.m_f; parity_id++) {

        // get vertex for parity block
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
        Vertex &bvtx = *getBlockVtx(block_meta);

        for (auto parity_relocation_candidate : parity_relocation_candidates) {
            // calculate the number of parity blocks with the same parity_id at the candidate node
            int num_parity_blocks_required = code.alpha;
            for (int stripe_id = 0; stripe_id < code.alpha; stripe_id++) {
                vector<int> &stripe_indices = stripes[stripe_id].getStripeIndices();
                if (stripe_indices[code.k_i + parity_id] == parity_relocation_candidate) {
                    num_parity_blocks_required -= 1;
                }
            }

            // get vertex for parity block relocation
            NodeMeta node_meta = {
                .id = -1,
                .node_id = parity_relocation_candidate,
                .vtx_id = -1
            };

            Vertex &nvtx = *getNodeVtx(node_meta);

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

    // Step 1: create a compute block (on left) to represent the dummy block to do re-encoding

    // Step 1.1 get vertex of compute block 
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
    Vertex &cb_vtx = *getBlockVtx(compute_block_meta);

    // Step 1.2: create edges to all candidate nodes for re-encoding. On a node with x data blocks, the edge is with cost (k_f - x)
    
    for (int node_id = 0; node_id < num_nodes; node_id++) {
        // number of data blocks required
        int num_data_blocks_required = code.k_f - num_data_stored[node_id];

        // get vertex of candidate nodes for re-encoding computation (all nodes are possible candidates)
        NodeMeta node_meta = {
            .id = -1,
            .node_id = node_id,
            .vtx_id = -1
        };
        Vertex &nvtx = *getNodeVtx(node_meta);

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


    // Step 2: for each parity block, create a compute node in the middle of the bipartite graph to represent re-encoding, and connect it with the parity block and the candidate relocation node. (p -> cn -> n)

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
        Vertex &bvtx = *getBlockVtx(parity_block_meta);

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
        Vertex &cn_vtx = *getBlockVtx(compute_node_meta);

        // 2.3 add edge to connect the parity block and compute node
        Edge edge = {
            .id = -1,
            .lvtx = &bvtx,
            .rvtx = &cn_vtx,
            .weight = 1,
            .cost = 0 // cost: no cost from parity block to compute block
        };
        edge.id = edges_map.size();
        edges_map[edge.id] = edge;

        // 2.4 for each parity block relocation candidate, connect it with compute node

        for (auto parity_relocation_candidate : parity_relocation_candidates) {
            // find vertex of the node
            NodeMeta node_meta = {
                .id = -1,
                .node_id = parity_relocation_candidate,
                .vtx_id = -1
            };
            Vertex &nvtx = *getNodeVtx(node_meta);

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
        BlockMeta &block_meta = *(it->second);
        printf("id: %d, type: %d, vtx_id: %d, stripe_batch_id: %d, stripe_group_id: %d, stripe_id: %d, stripe_id (global): %d, block_id: %d\n", block_meta.id, block_meta.type, block_meta.vtx_id, block_meta.stripe_batch_id, block_meta.stripe_group_id, block_meta.stripe_id, block_meta.stripe_id_global, block_meta.block_id);
    }

    printf("parity_block_meta:\n");
    for (auto it = parity_block_meta_map.begin(); it != parity_block_meta_map.end(); it++) {
        BlockMeta &block_meta = *(it->second);
        printf("id: %d, type: %d, vtx_id: %d, stripe_batch_id: %d, stripe_group_id: %d, stripe_id: %d, stripe_id (global): %d, block_id: %d\n", block_meta.id, block_meta.type, block_meta.vtx_id, block_meta.stripe_batch_id, block_meta.stripe_group_id, block_meta.stripe_id, block_meta.stripe_id_global, block_meta.block_id);
    }

    printf("compute_block_meta:\n");
    for (auto it = compute_block_meta_map.begin(); it != compute_block_meta_map.end(); it++) {
        BlockMeta &block_meta = *(it->second);
        printf("id: %d, type: %d, vtx_id: %d, stripe_batch_id: %d, stripe_group_id: %d, stripe_id: %d, stripe_id (global): %d, block_id: %d\n", block_meta.id, block_meta.type, block_meta.vtx_id, block_meta.stripe_batch_id, block_meta.stripe_group_id, block_meta.stripe_id, block_meta.stripe_id_global, block_meta.block_id);
    }

    printf("compute_node_meta:\n");
    for (auto it = compute_node_meta_map.begin(); it != compute_node_meta_map.end(); it++) {
        BlockMeta &block_meta = *(it->second);
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
    print_vertices(left_vertices_map);

    printf("Right vertices:\n");
    print_vertices(right_vertices_map);

    printf("Internal vertices:\n");
    print_vertices(internal_vertices_map);

    printf("Edges: \n");
    print_edges(edges_map);
}

Vertex *RecvBipartite::getBlockVtx(BlockMeta &in_block_meta) {


    // 2. get vertex of data block 
    int bvtx_id = -1;

    for (auto it = block_meta_map.begin(); it != block_meta_map.end(); it++) {
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
        // add a block vertex
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
        if (in_block_meta.type == DATA_BLK || in_block_meta.type == PARITY_BLK || in_block_meta.type == COMPUTE_BLK) {
            left_vertices_map[bvtx_id] = &vertices_map[bvtx_id];
        } else if (in_block_meta.type == COMPUTE_NODE) {
            internal_vertices_map[bvtx_id] = &vertices_map[bvtx_id];
        }

        // add block metadata
        BlockMeta new_block_meta = in_block_meta;
        new_block_meta.id = block_meta_map.size();
        new_block_meta.vtx_id = bvtx_id;
        block_meta_map[new_block_meta.id] = new_block_meta;

        if (in_block_meta.type == DATA_BLK) {
            data_block_meta_map[new_block_meta.id] = &block_meta_map[new_block_meta.id];
        } else if (in_block_meta.type == PARITY_BLK) {
            parity_block_meta_map[new_block_meta.id] = &block_meta_map[new_block_meta.id];
        } else if (in_block_meta.type == COMPUTE_BLK) {
            compute_block_meta_map[new_block_meta.id] = &block_meta_map[new_block_meta.id];
        } else if (in_block_meta.type == COMPUTE_NODE) {
            compute_node_meta_map[new_block_meta.id] = &block_meta_map[new_block_meta.id];
        }
    }

    if (in_block_meta.type == DATA_BLK || in_block_meta.type == PARITY_BLK || in_block_meta.type == COMPUTE_BLK) {
        return left_vertices_map[bvtx_id];
    } else if (in_block_meta.type == COMPUTE_NODE) {
        return internal_vertices_map[bvtx_id];
    } else {
        return NULL;
    }
}

Vertex *RecvBipartite::getNodeVtx(NodeMeta &in_node_meta) {
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
        

        NodeMeta new_node_meta = in_node_meta;
        new_node_meta.id = node_meta_map.size();
        new_node_meta.vtx_id = nvtx_id;
        node_meta_map[new_node_meta.id] = new_node_meta;
    }

    return right_vertices_map[nvtx_id];
}

BlockMeta *RecvBipartite::getBlockMeta(int vtx_id) {
    for (auto it = block_meta_map.begin(); it != block_meta_map.end(); it++) {
        BlockMeta &block_meta = it->second;
        if (block_meta.vtx_id == vtx_id) {
            return &it->second;
        }
    }
    return NULL;
}

NodeMeta *RecvBipartite::getNodeMeta(int vtx_id) {
    for (auto it = node_meta_map.begin(); it != node_meta_map.end(); it++) {
        NodeMeta &node_meta = it->second;
        if (node_meta.vtx_id == vtx_id) {
            return &it->second;
        }
    }
    return NULL;
}

bool RecvBipartite::BFSGraphForRecvGraph(RecvBipartite &recv_bipartite, int sid, int tid, int num_vertices, int **graph, int **res_graph, vector<int> &parent, map<int, vector<int>> &cur_reloc_node_map) {
    if (graph == NULL || res_graph == NULL || num_vertices < 0 || sid < 0 || sid > num_vertices || tid < 0 || tid > num_vertices) {
        printf("invalid input paramers\n");
        return false;
    }

    // mark all vertices as not visited
    vector<bool> visited(num_vertices, 0);

    queue<int> q;
    q.push(sid);

    visited[sid] = true; // mark source as visited

    // initialize the parent path
    for (auto &item : parent) {
        item = 0;
    }
    parent[sid] = -1;
    
    while (!q.empty()) {

        // get the top vertex traversed as vertex u
        int uid = q.front();
        q.pop();

        // traverse every vertex as vertex v
        for (int vid = 0; vid < num_vertices; vid++) {
            if (visited[vid] == false && res_graph[uid][vid] > 0) {

                parent[vid] = uid; // mark the parent of v as u
                if (vid == tid) { // we have reached the sink node t

                    /**
                     * @brief validate the path for recv graph
                     * requirement: each node should store only one block from one stripe group; if in previous found paths, we already store one block at one node, we cannot store another one
                    */

                   // parse into path
                    vector<int> path;
                    for (int vtx_id = tid; vtx_id != sid; vtx_id = parent[vtx_id]) {
                        path.push_back(vtx_id);
                    }
                    path.push_back(sid);
                    reverse(path.begin(), path.end());

                    // find metadata of the block and node
                    BlockMeta &block_meta = *(recv_bipartite.getBlockMeta(path[1]));
                    NodeMeta &node_meta = *(recv_bipartite.getNodeMeta(path[path.size() - 2]));

                    int stripe_group_id = block_meta.stripe_group_id;
                    if (stripe_group_id != -1 && (block_meta.type == DATA_BLK || block_meta.type == PARITY_BLK)) {
                        int reloc_node_id = node_meta.node_id;
                        vector<int> &cur_sg_reloc_nodes = cur_reloc_node_map[stripe_group_id];
                        // check if the node is already relocated with a node
                        if (find(cur_sg_reloc_nodes.begin(), cur_sg_reloc_nodes.end(), reloc_node_id) != cur_sg_reloc_nodes.end()) {
                            continue;
                        }
                    }

                    return true;
                }
                q.push(vid); // add the vertex to the queue
                visited[vid] = true;
            }
        }
    }

    // we cannot reach the sink node t
    return false;

}


int RecvBipartite::findMaxflowByFordFulkersonForRecvGraph(RecvBipartite &recv_bipartite, vector<vector<int>> &paths, int l_limit, int r_limit) {
    if (recv_bipartite.vertices_map.empty() == true || recv_bipartite.edges_map.empty() == true || l_limit <= 0 || r_limit <= 0) {
        printf("invalid input parameters\n");
        return -1;
    }

    // clear the output paths
    paths.clear();
    vector<vector<int>> initial_paths;

    // create two dummy nodes representing the source and target(sink) node
    int src_idx = recv_bipartite.vertices_map.size();
    int sink_idx = recv_bipartite.vertices_map.size() + 1;
    int num_vertices = recv_bipartite.vertices_map.size() + 2;
    
    // create graph weight and cost matrix (dim: (num_vertices + 2))
    int **graph_weight_mtx = Utils::init_int_matrix(num_vertices, num_vertices);
    int **graph_cost_mtx = Utils::init_int_matrix(num_vertices, num_vertices);
    
    for (auto it = recv_bipartite.edges_map.begin(); it != recv_bipartite.edges_map.end(); it++) {
        Edge &edge = it->second;
        graph_weight_mtx[edge.lvtx->id][edge.rvtx->id] = edge.weight;
        graph_cost_mtx[edge.lvtx->id][edge.rvtx->id] = edge.cost;
        graph_cost_mtx[edge.rvtx->id][edge.lvtx->id] = -edge.cost;
    }

    // add virtual edges from source node to left vertices. Edges are with weight = l_limit and cost = 0
    for (auto it = recv_bipartite.left_vertices_map.begin(); it != recv_bipartite.left_vertices_map.end(); it++) {
        Vertex &lvtx = *(it->second);
        graph_weight_mtx[src_idx][lvtx.id] = l_limit;
        graph_cost_mtx[src_idx][lvtx.id] = 0;
        graph_cost_mtx[lvtx.id][src_idx] = 0;
    }

    // add virtual edges from right vertices to sink. Edges are with weight = r_limit and cost = 0
    for (auto it = recv_bipartite.right_vertices_map.begin(); it != recv_bipartite.right_vertices_map.end(); it++) {
        Vertex &rvtx = *(it->second);
        graph_weight_mtx[rvtx.id][sink_idx] = r_limit;
        graph_cost_mtx[rvtx.id][sink_idx] = 0;
        graph_cost_mtx[sink_idx][rvtx.id] = 0;
    }

    printf("recv graph_weight_matrix:\n");
    Utils::print_int_matrix(graph_weight_mtx, num_vertices, num_vertices);
    printf("recv graph_cost_matrix:\n");
    Utils::print_int_matrix(graph_cost_mtx, num_vertices, num_vertices);

    // create residual graphs for weight and cost by copying
    int **res_graph_weight_mtx = Utils::init_int_matrix(num_vertices, num_vertices);
    int **res_graph_cost_mtx = Utils::init_int_matrix(num_vertices, num_vertices);
    for (int rid = 0; rid < num_vertices; rid++) {
        memcpy(res_graph_weight_mtx[rid], graph_weight_mtx[rid], num_vertices * sizeof(int));
        memcpy(res_graph_cost_mtx[rid], graph_cost_mtx[rid], num_vertices * sizeof(int));
    }

    // printf("recv res_graph_weight_matrix:\n");
    // Utils::print_int_matrix(res_graph_weight_mtx, num_vertices, num_vertices);
    // printf("recv res_graph_cost_matrix:\n");
    // Utils::print_int_matrix(res_graph_cost_mtx, num_vertices, num_vertices);


    vector<int> parent(num_vertices, 0); // it stores path from source to sink by 
    int max_flow = 0; // set initial flow as 0

    // create a map: <stripe_id, nodes where the blocks are stored in current paths>
    map<int, vector<int>> cur_reloc_node_map;

    while (RecvBipartite::BFSGraphForRecvGraph(recv_bipartite, src_idx, sink_idx, num_vertices, graph_weight_mtx, res_graph_weight_mtx, parent, cur_reloc_node_map) == true) {

        // find the minimum residual capacity of the edges along the path from source to sink by BFS.
        int uid, vid; // dummy vertices
        int path_flow = INT_MAX;
        for (vid = sink_idx; vid != src_idx; vid = parent[vid]) { // reversely traverse the path from sink to source
            uid = parent[vid]; // u: parent of v
            path_flow = min(path_flow, res_graph_weight_mtx[uid][vid]);
        }

        if (path_flow > 0) {

            // update residual capacities of the edges and reverse edges along the path
            for (int vid = sink_idx; vid != src_idx; vid = parent[vid]) { // reversely traverse the path from sink to source
                uid = parent[vid]; // u: parent of v

                // update flows
                res_graph_weight_mtx[uid][vid] -= path_flow;
                res_graph_weight_mtx[vid][uid] += path_flow;
            }

            // add path flow to overall flow
            max_flow += path_flow;

            // parse into path
            vector<int> path;
            for (int vid = sink_idx; vid != src_idx; vid = parent[vid]) {
                path.push_back(vid);
            }
            path.push_back(src_idx);
            reverse(path.begin(), path.end());

            // store the src to sink path to paths
            initial_paths.push_back(path);

            // mark the current destination node as relocated (with a block)
            // find metadata of the block and node
            BlockMeta &block_meta = *(recv_bipartite.getBlockMeta(path[1]));
            NodeMeta &node_meta = *(recv_bipartite.getNodeMeta(path[path.size() - 2]));

            int stripe_group_id = block_meta.stripe_group_id;
            if (stripe_group_id != -1 && (block_meta.type == DATA_BLK || block_meta.type == PARITY_BLK)) {
                int reloc_node_id = node_meta.node_id;
                vector<int> &cur_sg_reloc_nodes = cur_reloc_node_map[stripe_group_id];
                // mark the block is relocated
                cur_sg_reloc_nodes.push_back(reloc_node_id);
            }

        }
    }

    printf("res_graph_weight_mtx:\n");
    Utils::print_int_matrix(res_graph_weight_mtx, num_vertices, num_vertices);

    // currently, the residual weight graph stores the initially chosen paths, copy it out
    int cur_max_flow = max_flow;
    paths = initial_paths;

    // initialize the sink to src costs as infinite
    vector<int> t_to_s_costs(num_vertices, INT_MAX);
    t_to_s_costs[sink_idx] = 0; // dist(sink) = 0

    // repeat the iterations for |V| - 1 times
    for (int iter = 0; iter < num_vertices - 1; iter++) {
        // for every edge in residual graph, update t_to_s_costs
        for (int uid = 0; uid < num_vertices; uid++) {
            for (int vid = 0; vid < num_vertices; vid++) {

                // get edge (non-zero weight)
                int edge_weight = res_graph_weight_mtx[uid][vid];
                if (edge_weight == 0) {
                    continue;
                }

                // get edge cost
                int edge_cost = res_graph_cost_mtx[uid][vid];

                // there is an (non-zero cost) edge between u and v
                if (t_to_s_costs[uid] != INT_MAX && t_to_s_costs[vid] > t_to_s_costs[uid] + edge_cost) {
                    t_to_s_costs[vid] = t_to_s_costs[uid] + edge_cost;
                    printf("%d, %d, %d, %d\n", uid, vid, edge_weight, edge_cost);
                    printf("iter: %d, t_to_s_costs:\n", iter);
                    Utils::print_int_vector(t_to_s_costs);
                }
            }
        }
    }


    // now the t_to_s_costs stores the minimum distances to all nodes
    // check negative cost cycles
    for (int uid = 0; uid < num_vertices; uid++) {
        bool found_negative_cost = false;
        for (int vid = 0; vid < num_vertices; vid++) {
            // get edge (non-zero weight)
            int edge_weight = res_graph_weight_mtx[uid][vid];
            if (edge_weight == 0) {
                continue;
            }

            // get edge cost
            int edge_cost = res_graph_cost_mtx[uid][vid];
            if (t_to_s_costs[uid] != INT_MAX && t_to_s_costs[vid] > t_to_s_costs[uid] + edge_cost) {
                printf("find negative_cycles, (%d, %d, %d, %d, %d)\n", uid, vid, t_to_s_costs[uid], t_to_s_costs[vid], edge_cost);
                found_negative_cost = true;
                break;
            }
        }
        if (found_negative_cost == true) {
            break;
        }
    }

    return max_flow;
}
