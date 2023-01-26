#ifndef __BIPARTITE_HH__
#define __BIPARTITE_HH__

#include "../include/include.hh"
#include "StripeBatch.hh"


typedef struct Vertex {
    int id; // vertex id
    int in_degree; // sum of in-degree
    int out_degree; // sum of out-degree
    int weight; // sum of weights
    int costs; // sum of costs
} Vertex;

typedef struct Edge { // block to block edge
    int id;
    Vertex *lvtx; // left vertex
    Vertex *rvtx; // right vertex
    int weight; // weight of the edge
    int cost; // cost of the edge
} Edge;

class Bipartite
{
private:
    
public:
    map<int, Vertex> left_vertices_map; // left vertices map
    map<int, Vertex> right_vertices_map; // right vertices map

    map<int, Edge> edges_map; // block to node edges

    Bipartite();
    ~Bipartite();

    void clear();
};


#endif // __BIPARTITE_HH__