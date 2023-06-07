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
    vtx.id = new_vtx_id;
    vtx.type = type;

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
    vector<uint64_t> sol_edges;

    uint64_t num_lvtxes = left_vertices_map.size();
    uint64_t num_rvtxes = right_vertices_map.size();
    bool *adjacency_matrix = (bool *)calloc(num_lvtxes * num_rvtxes, sizeof(bool));            // adjacency matrix
    uint64_t *edge_id_adj_mat = (uint64_t *)calloc(num_lvtxes * num_rvtxes, sizeof(uint64_t)); // map edge_id with items in adjacency matrix
    bool *cur_sm_matrix = (bool *)calloc(num_lvtxes * num_rvtxes, sizeof(bool));               // current semi-matching matrix (record current semi-matching)

    // mark the adjacency matrix
    // lvtx: [idx: 0 to num_lvtxes - 1]
    // rvtx: [idx: 0 to num_rvtxes - 1]
    for (auto &item : edges_map)
    {
        uint64_t edge_id = item.first;
        uint64_t lvtx_id = item.second.lvtx_id;
        uint64_t rvtx_id = item.second.rvtx_id;
        adjacency_matrix[lvtx_id * num_rvtxes + rvtx_id] = true;   // mark the edge in adjacent matrix
        edge_id_adj_mat[lvtx_id * num_rvtxes + rvtx_id] = edge_id; // record edge_id for the item in adjacent matrix
    }

    // printf("adjacency matrix:\n");
    // for (uint64_t lvtx_id = 0; lvtx_id < num_lvtxes; lvtx_id++)
    // {
    //     for (uint64_t rvtx_id = 0; rvtx_id < num_rvtxes; rvtx_id++)
    //     {
    //         printf("%d ", adjacency_matrix[lvtx_id * num_rvtxes + rvtx_id]);
    //     }
    //     printf("\n");
    // }
    // printf("\n");

    // loop each left vertex (as root)
    for (uint64_t idx = 0; idx < num_lvtxes; idx++)
    {
        Vertex &root_lvtx = left_vertices_map[idx];

        // build alternating search tree with lvtx as the root
        queue<Vertex *> bfs_queue;
        bfs_queue.push(&root_lvtx);                   // enqueue root lvtx
        vector<bool> lvtx_visited(num_lvtxes, false); // record whether the lvtx is visited
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

                    assert(adjacency_matrix[vtx.id * num_rvtxes + rvtx_id] == true); // make sure the item in adjacency matrix is valid

                    // find an unmatched edge
                    if (cur_sm_matrix[vtx.id * num_rvtxes + rvtx_id] == false)
                    {
                        // we don't allow the lvtx connect to a rvtx which is already connected with a lvtx of the same stripe group, as alternating the paths within the two lvtxes in the same stripe group doesn't help reducing the load
                        bool is_rvtx_valid = true;

                        for (auto sg_lvtx : sg_lvtxes)
                        {
                            if (cur_sm_matrix[sg_lvtx * num_rvtxes + rvtx_id] == true) // rvtx already been matched by sg_lvtx of the same stripe group
                            {
                                is_rvtx_valid = false; // mark the edge as not valid
                                break;
                            }
                        }

                        // if the edge is unmatched
                        if (is_rvtx_valid == true)
                        {
                            neighbors.push_back(rvtx_id); // obtain unmatched rvtx
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
                    uint64_t lvtx_id = edges_map[edge_id].lvtx_id; // corresponding lvtx

                    assert(adjacency_matrix[lvtx_id * num_rvtxes + vtx.id] == true); // make sure the item in adjacency matrix is valid

                    if (cur_sm_matrix[lvtx_id * num_rvtxes + vtx.id] == true) // find matched edge
                    {
                        neighbors.push_back(lvtx_id); // obtain matched lvtx
                    }
                }

                // if currently least_load_rvtx is not selected, or current rvtx has lower load than least_load_rvtx, set current rvtx as the least load rvtx
                if (least_load_rvtx == NULL || vtx.in_degree < least_load_rvtx->in_degree)
                {
                    least_load_rvtx = &vtx;
                }
            }

            // loop the neighbors if they are not visited
            for (auto neighbor_vtx_id : neighbors)
            {
                Vertex *nvtx = NULL;
                if (vtx.type == VertexType::LEFT)
                {
                    // the neighbor is a rvtx
                    if (rvtx_visited[neighbor_vtx_id] == true) // skip visited neighbor
                    {
                        continue;
                    }
                    nvtx = &right_vertices_map[neighbor_vtx_id]; // obtain nvtx
                    rvtx_parent_map[neighbor_vtx_id] = vtx.id;   // mark parent
                    rvtx_visited[neighbor_vtx_id] = true;        // mark visited
                }
                else if (vtx.type == VertexType::RIGHT)
                {
                    // the neighbor is a lvtx
                    if (lvtx_visited[neighbor_vtx_id] == true) // skip visited neighbor
                    {
                        continue;
                    }
                    nvtx = &left_vertices_map[neighbor_vtx_id]; // obtain nvtx
                    lvtx_parent_map[neighbor_vtx_id] = vtx.id;  // mark parent
                    lvtx_visited[neighbor_vtx_id] = true;       // mark visited
                }

                bfs_queue.push(nvtx); // enqueue the neighbor
            }
        }

        // we already found an alternating path from root to selected_rvtx, thus we choose the edges along the path
        // for edges chosen before (in alternating path, it's pointed from rvtx to lvtx), we mark the cur_sm_matrix[lvtx][rvtx] as false; for newly added edge (in alternating path, it's pointed from lvtx to rvtx), we mark cur_sm_matrix[lvtx][rvtx] as true
        Vertex *v = least_load_rvtx;
        Vertex *u = &left_vertices_map[rvtx_parent_map[v->id]];
        cur_sm_matrix[u->id * num_rvtxes + v->id] = true;
        v->in_degree++; // increase load of v only
        while (u->id != root_lvtx.id)
        {
            // remove v->u
            v = &right_vertices_map[lvtx_parent_map[u->id]];
            cur_sm_matrix[u->id * num_rvtxes + v->id] = false;
            // add u->v
            u = &left_vertices_map[rvtx_parent_map[v->id]];
            cur_sm_matrix[u->id * num_rvtxes + v->id] = true;
        }
    }

    // record the edges in the semi-matching
    for (uint64_t lvtx_id = 0; lvtx_id < num_lvtxes; lvtx_id++)
    {
        for (uint64_t rvtx_id = 0; rvtx_id < num_rvtxes; rvtx_id++)
        {
            if (cur_sm_matrix[lvtx_id * num_rvtxes + rvtx_id] == true)
            {
                uint64_t edge_id = edge_id_adj_mat[lvtx_id * num_rvtxes + rvtx_id];
                sol_edges.push_back(edge_id);
            }
        }
    }

    free(adjacency_matrix);
    free(edge_id_adj_mat);
    free(cur_sm_matrix);

    return sol_edges;
}

void Bipartite::clear()
{
    left_vertices_map.clear();
    right_vertices_map.clear();
    edges_map.clear();
}

void Bipartite::print()
{
    printVertices();
    printEdges();
}

void Bipartite::printVertices()
{
    printf("left_vertices (size: %ld):\n", left_vertices_map.size());
    printVertices(left_vertices_map);

    printf("right_vertices (size: %ld):\n", right_vertices_map.size());
    printVertices(right_vertices_map);
}

void Bipartite::printVertices(unordered_map<uint64_t, Vertex> &vtx_map)
{
    for (auto it = vtx_map.begin(); it != vtx_map.end(); it++)
    {
        Vertex &vtx = it->second;
        printf("id: %ld, in_degree: %d, out_degree: %d\n", vtx.id, vtx.in_degree, vtx.out_degree);
    }
}

void Bipartite::printEdges()
{
    printf("edges (size: %ld):\n", edges_map.size());
    for (auto it = edges_map.begin(); it != edges_map.end(); it++)
    {
        Edge &edge = it->second;
        printf("id: %ld, lvtx(.id): %ld, rvtx(.id): %ld\n", edge.id, edge.lvtx_id, edge.rvtx_id);
    }
}