#include "Bipartite.hh"

Bipartite::Bipartite()
{
}

Bipartite::~Bipartite()
{
}

void Bipartite::clear() {
    left_vertices_map.clear();
    right_vertices_map.clear();
    edges_map.clear();
}