#include "Bipartite.hh"

Bipartite::Bipartite()
{
}

Bipartite::~Bipartite()
{
}

void Bipartite::clear() {
    vertices_map.clear();
    left_vertices_map.clear();
    right_vertices_map.clear();
    edges_map.clear();
}

void Bipartite::print() {
    
}
void Bipartite::print_vertices(map<int, Vertex> &vmap) {
    for (auto it = vmap.begin(); it != vmap.end(); it++) {
        Vertex &vtx = it->second;
        printf("id: %d, in_degree: %d, out_degree: %d, weights: %d, costs: %d\n", vtx.id, vtx.in_degree, vtx.out_degree, vtx.weights, vtx.costs);
    }
}
void Bipartite::print_vertices(map<int, Vertex *> &vmap) {
    for (auto it = vmap.begin(); it != vmap.end(); it++) {
        Vertex &vtx = *(it->second);
        printf("id: %d, in_degree: %d, out_degree: %d, weights: %d, costs: %d\n", vtx.id, vtx.in_degree, vtx.out_degree, vtx.weights, vtx.costs);
    }
}
    
void Bipartite::print_edges(map<int, Edge> &emap) {
    for (auto it = emap.begin(); it != emap.end(); it++) {
        int edge_id = it->first;
        Edge &edge = it->second;
        printf("id: %d, lvtx(.id): %d, rvtx(.id): %d, weight: %d, cost: %d\n", edge_id, edge.lvtx->id, edge.rvtx->id, edge.weight, edge.cost);
    }
}

int Bipartite::findMaxflowByFordFulkerson(Bipartite &bipartite, vector<vector<int>> &paths) {
    if (bipartite.vertices_map.empty() == true || bipartite.edges_map.empty() == true) {
        printf("invalid input graph\n");
        return -1;
    }

    // clear the output paths
    paths.clear();

    // create two dummy nodes representing the source and target(sink) node
    int src_idx = bipartite.vertices_map.size();
    int sink_idx = bipartite.vertices_map.size() + 1;
    int num_vertices = bipartite.vertices_map.size() + 2;
    
    // create graph weight and cost matrix (dim: (num_vertices + 2))
    int **graph_weight_mtx = Utils::init_int_matrix(num_vertices, num_vertices);
    int **graph_cost_mtx = Utils::init_int_matrix(num_vertices, num_vertices);
    
    for (auto it = bipartite.edges_map.begin(); it != bipartite.edges_map.end(); it++) {
        Edge &edge = it->second;
        graph_weight_mtx[edge.lvtx->id][edge.rvtx->id] = edge.weight;
        graph_cost_mtx[edge.lvtx->id][edge.rvtx->id] = edge.cost;
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

    vector<int> parent(num_vertices, 0); // it stores path from source to sink by 
    int max_flow = 0; // set initial flow as 0

    while (Bipartite::BFSGraph(src_idx, sink_idx, num_vertices, graph_weight_mtx, res_graph_weight_mtx, parent) == true) {
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
                res_graph_weight_mtx[uid][vid] -= path_flow;
                res_graph_weight_mtx[vid][uid] += path_flow;            
            }

            // add path flow to overall flow
            max_flow += path_flow;

            // store the src to sink path to paths
            paths.push_back(parent);
        }
    }

    return max_flow;
}

bool Bipartite::BFSGraph(int sid, int tid, int num_vertices, int **graph, int **res_graph, vector<int> &parent) {
    if (graph == NULL || res_graph == NULL || num_vertices < 0 || sid < 0 || sid > num_vertices || tid < 0 || tid > num_vertices) {
        printf("invalid input paramers\n");
        return false;
    }

    parent.clear(); // clear the parent path

    // mark all vertices as not visited
    vector<bool> visited(num_vertices, 0);

    queue<int> q;
    q.push(sid);

    visited[sid] = true; // mark source as visited
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

bool Bipartite::buildMaxFlowSolutionFromPaths(vector<vector<int>> &paths) {
    // clear all edges
    edges_max_flow_map.clear();

    for (auto path : paths) {
        if (path.size() < 2) {
            printf("error: path of invalid length\n");
            return false;
        }

        for (int vtx_idx = 0; vtx_idx < path.size() - 1; vtx_idx++) {
            int uid = path[vtx_idx];
            int vid = path[vtx_idx + 1];

            for (auto it = edges_map.begin(); it != edges_map.end(); it++) {
                Edge &edge = it->second;
                // copy the edge
                if (edge.lvtx->id == uid && edge.rvtx->id == vid) {
                    edges_max_flow_map[it->first] = Edge();
                    memcpy(&edges_max_flow_map[it->first], &edge, sizeof(Edge));
                }
            }
        }
    }

    return true;
}