#include "Bipartite.hh"

Bipartite::Bipartite()
{
}

Bipartite::~Bipartite()
{
}

size_t Bipartite::addVertex(VertexType type) {
    unordered_map<int, Vertex *> *vertex_map_ptr = NULL;
    if (type == VertexType::LEFT) {
        // left vertex
        vertex_map_ptr = &left_vertices_map;
    } else if (type == VertexType::RIGHT) {
        // right vertex
        vertex_map_ptr = &right_vertices_map;
    } else {
        printf("invalid vertex type: %d\n", type);
        return -1;
    }

    // create new vertex
    size_t new_vtx_id = vertices.size(); // vertex id
    vertices.push_back(Vertex());
    Vertex &vertex = vertices[new_vtx_id];
    vertex.id = new_vtx_id;

    // create reference in vertex map
    (*vertex_map_ptr)[new_vtx_id] = &vertex;

    return new_vtx_id;
}

size_t Bipartite::addEdge(Vertex *lvtx, Vertex *rvtx, int weight, int cost) {

    if (lvtx->id >= vertices.size() || rvtx->id >= vertices.size()) {
        printf("vertex does not exist\n");
        return -1;
    }

    // create new vertex
    size_t new_edge_id = edges.size(); // edge id
    edges.push_back(Edge());
    Edge &edge = edges[new_edge_id];
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
    vertices.clear();
    left_vertices_map.clear();
    right_vertices_map.clear();
    edges.clear();
}

void Bipartite::print() {
    print_vertices();
    print_edges();
}

void Bipartite::print_vertices() {
    printf("left_vertices:\n");
    print_vertices(left_vertices_map);

    printf("right_vertices:\n");
    print_vertices(right_vertices_map);
}

void Bipartite::print_vertices(unordered_map<int, Vertex *> &vertex_map) {
    for (auto it = vertex_map.begin(); it != vertex_map.end(); it++) {
        Vertex &vtx = *(it->second);
        printf("id: %d, in_degree: %d, out_degree: %d, weights: %d, costs: %d\n", vtx.id, vtx.in_degree, vtx.out_degree, vtx.weights, vtx.costs);
    }
}
    
void Bipartite::print_edges() {
    printf("edges:\n");
    for (auto &edge : edges) {
        printf("id: %d, lvtx(.id): %d, rvtx(.id): %d, weight: %d, cost: %d\n", edge.id, edge.lvtx->id, edge.rvtx->id, edge.weight, edge.cost);
    }
}