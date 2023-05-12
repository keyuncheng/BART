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
    VertexType type; // vertex type
    int in_degree;   // sum of in-degree
    int out_degree;  // sum of out-degree

    Vertex() : id(INVALID_VTX_ID), type(INVALID_VTX), in_degree(0), out_degree(0){};
} Vertex;

typedef struct Edge
{ // block to block edge
    uint64_t id;
    uint64_t lvtx_id; // left vertex id
    uint64_t rvtx_id; // right vertex id

    Edge() : id(INVALID_VTX_ID), lvtx_id(INVALID_VTX_ID), rvtx_id(INVALID_VTX_ID){};
} Edge;

class Bipartite
{
private:
public:
    Bipartite();
    ~Bipartite();
    uint64_t addVertex(VertexType type); // 0: left_vertex; 1: right vertex; return:  new vertex id
    uint64_t addEdge(uint64_t lvtx_id, uint64_t rvtx_id);

    vector<uint64_t> findOptSemiMatching(unordered_map<uint64_t, pair<uint32_t, uint8_t>> &lvtx2sg_map, unordered_map<uint32_t, vector<uint64_t>> &sg2lvtx_map); // find optimal semi-matching with stripe group information

    void clear();
    void print();
    void print_vertices();
    void print_vertices(unordered_map<uint64_t, Vertex> &vertex_map);
    void print_edges();

    unordered_map<uint64_t, Vertex> left_vertices_map;  // left vertices map
    unordered_map<uint64_t, Vertex> right_vertices_map; // right vertices map
    unordered_map<uint64_t, Edge> edges_map;

    unordered_map<uint64_t, vector<uint64_t>> lvtx_edges_map; // edges connected to left vertices <lvtx, <edges>>
    unordered_map<uint64_t, vector<uint64_t>> rvtx_edges_map; // edges connected to right vertices <rvtx, <edges>>
};

#endif // __BIPARTITE_HH__