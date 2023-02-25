#ifndef __BIPARTITE_HH__
#define __BIPARTITE_HH__

#include <queue>

#include "../include/include.hh"
#include "StripeBatch.hh"
#include "../util/Utils.hh"

enum VertexType {
    LEFT,
    RIGHT,
    INVALID
};

typedef struct Vertex {
    size_t id;
    int in_degree; // sum of in-degree
    int out_degree; // sum of out-degree
    int weights; // sum of weights
    int costs; // sum of costs

    Vertex() : id(INVALID_ID), in_degree(0), out_degree(0), weights(0), costs(0) {};
} Vertex;

typedef struct Edge { // block to block edge
    size_t id;
    Vertex *lvtx; // left vertex
    Vertex *rvtx; // right vertex
    int weight; // weight of the edge
    int cost; // cost of the edge

    Edge() : id(INVALID_ID), lvtx(NULL), rvtx(NULL), weight(0), cost(0) {};
} Edge;

class Bipartite
{
private:
    
public:
    Bipartite();
    ~Bipartite();
    size_t addVertex(VertexType type); // 0: left_vertex; 1: right vertex; return:  new vertex id
    size_t addEdge(Vertex *lvtx, Vertex *rvtx, int weight, int cost);

    void clear();
    void print();
    void print_vertices();
    void print_vertices(unordered_map<size_t, Vertex *> &vertex_map);
    void print_edges();


    unordered_map<size_t, Vertex> vertices_map;
    unordered_map<size_t, Edge> edges_map;

    unordered_map<size_t, Vertex *> left_vertices_map; // left vertices map
    unordered_map<size_t, Vertex *> right_vertices_map; // right vertices map

    unordered_map<size_t, vector<size_t> > lvtx_edges_map; // edges connected to left vertices <lvtx, <edges>>
    unordered_map<size_t, vector<size_t> > rvtx_edges_map; // edges connected to right vertices <rvtx, <edges>>

};


#endif // __BIPARTITE_HH__