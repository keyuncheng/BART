#ifndef __BIPARTITE_HH__
#define __BIPARTITE_HH__

#include "../include/include.hh"
#include "StripeBatch.hh"

typedef struct BlockVtx {
    int id;
    int stripe_batch_id;
    int stripe_group_id;
    int stripe_id;
    int stripe_id_global;
    int block_id;
    int in_degree;
    int out_degree;
    int costs;
} BlockVtx;

typedef struct NodeVtx {
    int id;
    int node_id;
    int in_degree;
    int out_degree;
    int costs;
} NodeVtx;

typedef struct BEdge {
    int id;
    BlockVtx *lvtx; // left vertex
    NodeVtx *rvtx; // right vertex
    int weight; // weight of the edge (in current implementation, weight is fixed to 1)
    int cost; // cost of the edge
} Edge;

class Bipartite
{
private:
    
public:
    map<int, BlockVtx> left_vertices_map;
    map<int, NodeVtx> right_vertices_map;
    map<int, BEdge> edges_map;

    Bipartite();
    ~Bipartite();

    void clear();
};


#endif // __BIPARTITE_HH__