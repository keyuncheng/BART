#ifndef __BIPARTITE_HH__
#define __BIPARTITE_HH__

#include "../include/include.hh"
#include "StripeBatch.hh"

typedef struct BEdge {
    int lvtx; // left vertex
    int rvtx; // right vertex
    int weight; // weight of the edge
} Edge;

class Bipartite
{
private:
    
public:
    StripeBatch *stripe_batch;
    vector<int> left_vertices;
    vector<int> right_vertices;
    vector<BEdge> edges;

    Bipartite();
    ~Bipartite();
};


#endif // __BIPARTITE_HH__