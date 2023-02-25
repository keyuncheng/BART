#include "Bipartite.hh"

Bipartite::Bipartite()
{
}

Bipartite::~Bipartite()
{
}

size_t Bipartite::addVertex(VertexType type) {
    unordered_map<size_t, Vertex *> *vtx_map_ptr = NULL;
    if (type == VertexType::LEFT) { // left vertex
        vtx_map_ptr = &left_vertices_map;
    } else if (type == VertexType::RIGHT) { // right vertex
        vtx_map_ptr = &right_vertices_map;
    } else {
        printf("invalid vertex type: %d\n", type);
        return INVALID_ID;
    }

    // create new vertex
    size_t new_vtx_id = vertices_map.size(); // vertex id
    vertices_map[new_vtx_id].id = new_vtx_id;

    // create reference in vertex map
    (*vtx_map_ptr)[new_vtx_id] = &vertices_map[new_vtx_id];

    return new_vtx_id;
}

size_t Bipartite::addEdge(Vertex *lvtx, Vertex *rvtx, int weight, int cost) {

    // create new vertex
    size_t new_edge_id = edges_map.size(); // edge id
    Edge &edge = edges_map[new_edge_id];
    edge.id = new_edge_id;
    edge.lvtx = lvtx;
    edge.rvtx = rvtx;
    edge.weight = weight;
    edge.cost = cost;

    // create references
    lvtx_edges_map[lvtx->id].push_back(edge.id);
    rvtx_edges_map[rvtx->id].push_back(edge.id);
    
    return new_edge_id;
}

void Bipartite::clear() {
    vertices_map.clear();
    left_vertices_map.clear();
    right_vertices_map.clear();
    edges_map.clear();
}

void Bipartite::print() {
    print_vertices();
    print_edges();
}

void Bipartite::print_vertices() {
    printf("left_vertices (size: %ld):\n", left_vertices_map.size());
    print_vertices(left_vertices_map);

    printf("right_vertices (size: %ld):\n", right_vertices_map.size());
    print_vertices(right_vertices_map);
}

void Bipartite::print_vertices(unordered_map<size_t, Vertex *> &vtx_map) {
    for (auto it = vtx_map.begin(); it != vtx_map.end(); it++) {
        Vertex &vtx = *(it->second);
        printf("id: %ld, in_degree: %d, out_degree: %d, weights: %d, costs: %d\n", vtx.id, vtx.in_degree, vtx.out_degree, vtx.weights, vtx.costs);
    }
}
    
void Bipartite::print_edges() {
    printf("edges (size: %ld):\n", edges_map.size());
    for (auto it = edges_map.begin(); it != edges_map.end(); it++) {
        Edge &edge = it->second;
        printf("id: %ld, lvtx(.id): %ld, rvtx(.id): %ld, weight: %d, cost: %d\n", edge.id, edge.lvtx->id, edge.rvtx->id, edge.weight, edge.cost);
    }
}