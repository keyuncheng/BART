#include "Bipartite.hh"

Bipartite::Bipartite()
{
}

Bipartite::~Bipartite()
{
}

uint64_t Bipartite::addVertex(VertexType type)
{
    unordered_map<uint64_t, Vertex> *vtx_map_ptr = NULL;
    if (type == VertexType::LEFT)
    { // left vertex
        vtx_map_ptr = &left_vertices_map;
    }
    else if (type == VertexType::RIGHT)
    { // right vertex
        vtx_map_ptr = &right_vertices_map;
    }
    else
    {
        printf("invalid vertex type: %d\n", type);
        return INVALID_VTX_ID;
    }

    // create new vertex
    uint64_t new_vtx_id = vtx_map_ptr->size(); // vertex id
    Vertex &vtx = (*vtx_map_ptr)[new_vtx_id];
    (*vtx_map_ptr)[new_vtx_id].id = new_vtx_id;
    (*vtx_map_ptr)[new_vtx_id].type = type;

    return new_vtx_id;
}

uint64_t Bipartite::addEdge(uint64_t lvtx_id, uint64_t rvtx_id)
{

    // create new vertex
    uint64_t new_edge_id = edges_map.size(); // edge id
    Edge &edge = edges_map[new_edge_id];
    edge.id = new_edge_id;
    edge.lvtx_id = lvtx_id;
    edge.rvtx_id = rvtx_id;

    // create references
    lvtx_edges_map[lvtx_id].push_back(edge.id);
    rvtx_edges_map[rvtx_id].push_back(edge.id);

    return new_edge_id;
}

vector<uint64_t> Bipartite::findOptSemiMatching(unordered_map<uint64_t, pair<uint32_t, uint8_t>> &lvtx2sg_map, unordered_map<uint32_t, vector<uint64_t>> &sg2lvtx_map)
{
    uint64_t num_lvtxes = left_vertices_map.size();
    uint64_t num_rvtxes = right_vertices_map.size();
    bool *adjacency_matrix = (bool *)calloc(num_lvtxes * num_rvtxes, sizeof(bool)); // adjacency matrix
    bool *cur_sm_matrix = (bool *)calloc(num_lvtxes * num_rvtxes, sizeof(bool));    // current semi-matching matrix (record current semi-matching)

    // mark the adjacency matrix
    // lvtx: [idx: 0 to num_lvtxes - 1]
    // rvtx: [idx: 0 to num_rvtxes - 1], corresponding to node 0 to num_nodes - 1
    for (auto &item : edges_map)
    {
        uint64_t lvtx_id = item.second.lvtx_id;
        uint64_t rvtx_id = item.second.rvtx_id;
        adjacency_matrix[lvtx_id * num_lvtxes + rvtx_id] = true; // mark the edge in adjacent matrix
    }

    // for each left vertex (root)
    for (uint64_t idx = 0; idx < num_lvtxes; idx++)
    {
        Vertex &root_lvtx = left_vertices_map[idx];
        uint64_t sg_id = lvtx2sg_map[root_lvtx.id].first;

        vector<uint64_t> &sg_lvtxes = sg2lvtx_map[sg_id]; // lvtxes of the same stripe group; **no two lvtxes** can be matched to the same node

        // build alternating search tree with lvtx as the root
        queue<Vertex *> bfs_queue;
        bfs_queue.push(&root_lvtx); // enqueue root
        vector<bool> lvtx_visited(num_lvtxes, false);
        vector<bool> rvtx_visited(num_rvtxes, false);
        Vertex *least_load_rvtx = NULL;                    // least load rvtx
        unordered_map<uint64_t, uint64_t> lvtx_parent_map; // trace the alternating path
        unordered_map<uint64_t, uint64_t> rvtx_parent_map; // trace the alternating path

        while (bfs_queue.empty() == false)
        {
            Vertex &vtx = *bfs_queue.front();
            bfs_queue.pop();
            vector<uint64_t> neighbors; // for lvtxes, find unmatched neighbors; for rvtxes, find matched neighbors

            // check if it's left or right vertex
            if (vtx.type == VertexType::LEFT)
            {
                uint64_t sg_id = lvtx2sg_map[vtx.id].first;
                vector<uint64_t> sg_lvtxes = sg2lvtx_map[sg_id];
                // find unmatched rvtxes, which is not connected by lvtxes of the same stripe group
                vector<uint64_t> &connected_edges = lvtx_edges_map[vtx.id];
                for (auto edge_id : connected_edges)
                {
                    uint64_t rvtx_id = edges_map[edge_id].rvtx_id;
                    if (adjacency_matrix[vtx.id * num_lvtxes + rvtx_id] == false) // unmatched vertex
                    {
                        // check if it's not matched by lvtxes of the same stripe group
                        if (find(sg_lvtxes.begin(), sg_lvtxes.end(), rvtx_id) == sg_lvtxes.end())
                        {
                            // add to unmatched neighbors
                            neighbors.push_back(rvtx_id);
                        }
                    }
                }
            }
            else if (vtx.type == VertexType::RIGHT)
            {
                // find matched lvtxes, which is connected by some lvtxes
                vector<uint64_t> &connected_edges = rvtx_edges_map[vtx.id];
                for (auto edge_id : connected_edges)
                {
                    uint64_t rvtx_id = edges_map[edge_id].rvtx_id;
                    if (adjacency_matrix[vtx.id * num_lvtxes + rvtx_id] == true) // matched vertex
                    {
                        neighbors.push_back(rvtx_id);
                    }
                }

                // if no least load rvtx is selected, or current rvtx has less load than least load vtx, set current rvtx as the least load rvtx
                if (least_load_rvtx == NULL || vtx.in_degree < least_load_rvtx->in_degree)
                {
                    least_load_rvtx = &vtx;
                }
            }

            // loop each neighbor
            for (auto neighbor_vtx_id : neighbors)
            {
                Vertex *nvtx = NULL;
                if (vtx.type == VertexType::LEFT)
                {
                    rvtx_parent_map[neighbor_vtx_id] = vtx.id;   // mark parent
                    nvtx = &right_vertices_map[neighbor_vtx_id]; // nvtx is a rvtx
                    rvtx_visited[neighbor_vtx_id] = true;        // mark visited
                }
                else if (vtx.type == VertexType::RIGHT)
                {
                    lvtx_parent_map[neighbor_vtx_id] = vtx.id;  // mark parent
                    nvtx = &left_vertices_map[neighbor_vtx_id]; // nvtx is a lvtx
                    lvtx_visited[neighbor_vtx_id] = true;       // mark visited
                }
                bfs_queue.push(nvtx); // enqueue the neighbor
            }
        }

        // we already found an alternating path from selected_rvtx to root, thus we choose the edges
        // for edges chosen before (in alternating path, it's pointed from rvtx to lvtx), we mark the adjacency[lvtx][rvtx] as false; otherwise, we mark adjacency[lvtx][rvtx] as true
        Vertex *v = least_load_rvtx;
        Vertex *u = &left_vertices_map[rvtx_parent_map[v->id]];
        adjacency_matrix[u->id * num_lvtxes + v->id] = true;
        while (u->id != root_lvtx.id)
        {
            // remove v->u
            v = &right_vertices_map[lvtx_parent_map[u->id]];
            adjacency_matrix[u->id * num_lvtxes + v->id] = false;
            // add u->v
            u = &left_vertices_map[rvtx_parent_map[v->id]];
            adjacency_matrix[u->id * num_lvtxes + v->id] = true;
        }
    }

    free(adjacency_matrix);
    free(cur_sm_matrix);

    return vector<uint64_t>();
}

void Bipartite::clear()
{
    left_vertices_map.clear();
    right_vertices_map.clear();
    edges_map.clear();
}

void Bipartite::print()
{
    print_vertices();
    print_edges();
}

void Bipartite::print_vertices()
{
    printf("left_vertices (size: %ld):\n", left_vertices_map.size());
    print_vertices(left_vertices_map);

    printf("right_vertices (size: %ld):\n", right_vertices_map.size());
    print_vertices(right_vertices_map);
}

void Bipartite::print_vertices(unordered_map<uint64_t, Vertex> &vtx_map)
{
    for (auto it = vtx_map.begin(); it != vtx_map.end(); it++)
    {
        Vertex &vtx = it->second;
        printf("id: %ld, in_degree: %d, out_degree: %d\n", vtx.id, vtx.in_degree, vtx.out_degree);
    }
}

void Bipartite::print_edges()
{
    printf("edges (size: %ld):\n", edges_map.size());
    for (auto it = edges_map.begin(); it != edges_map.end(); it++)
    {
        Edge &edge = it->second;
        printf("id: %ld, lvtx(.id): %ld, rvtx(.id): %ld\n", edge.id, edge.lvtx_id, edge.rvtx_id);
    }
}