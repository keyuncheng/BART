#ifndef __BIPARTITE_HH__
#define __BIPARTITE_HH__

#include <queue>

#include "../include/include.hh"
#include "StripeBatch.hh"
#include "../util/Utils.hh"

enum VertexType
{
    LEFT,
    RIGHT,
    INVALID_VTX
};

typedef struct Vertex
{
    uint64_t id;
    int in_degree;  // sum of in-degree
    int out_degree; // sum of out-degree

    Vertex() : id(INVALID_VTX_ID), in_degree(0), out_degree(0){};
} Vertex;

typedef struct Edge
{ // block to block edge
    uint64_t id;
    Vertex *lvtx; // left vertex
    Vertex *rvtx; // right vertex

    Edge() : id(INVALID_VTX_ID), lvtx(NULL), rvtx(NULL){};
} Edge;

class Bipartite
{
private:
public:
    Bipartite();
    ~Bipartite();
    uint64_t addVertex(VertexType type); // 0: left_vertex; 1: right vertex; return:  new vertex id
    uint64_t addEdge(Vertex *lvtx, Vertex *rvtx);

    vector<uint64_t> findOptSemiMatching(); // find optimal semi-matching, return edge_ids of the semi-matching

    void clear();
    void print();
    void print_vertices();
    void print_vertices(unordered_map<uint64_t, Vertex *> &vertex_map);
    void print_edges();

    unordered_map<uint64_t, Vertex> vertices_map;
    unordered_map<uint64_t, Edge> edges_map;

    unordered_map<uint64_t, Vertex *> left_vertices_map;  // left vertices map
    unordered_map<uint64_t, Vertex *> right_vertices_map; // right vertices map

    unordered_map<uint64_t, vector<uint64_t>> lvtx_edges_map; // edges connected to left vertices <lvtx, <edges>>
    unordered_map<uint64_t, vector<uint64_t>> rvtx_edges_map; // edges connected to right vertices <rvtx, <edges>>
};

#endif // __BIPARTITE_HH__