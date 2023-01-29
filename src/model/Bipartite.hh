#ifndef __BIPARTITE_HH__
#define __BIPARTITE_HH__

#include <queue>

#include "../include/include.hh"
#include "StripeBatch.hh"
#include "../util/Utils.hh"

typedef struct Vertex {
    int id; // vertex id
    int in_degree; // sum of in-degree
    int out_degree; // sum of out-degree
    int weights; // sum of weights
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
    map<int, Vertex> vertices_map; // vertices map
    map<int, Edge> edges_map; // block to node edges

    map<int, Vertex *> left_vertices_map; // left vertices map
    map<int, Vertex *> right_vertices_map; // right vertices map

    
    // max flow solution
    map<int, Edge> edges_max_flow_map; // edges of max flow solution


    Bipartite();
    ~Bipartite();

    void print();
    void print_vertices(map<int, Vertex> &vmap);
    void print_vertices(map<int, Vertex *> &vmap);
    void print_edges(map<int, Edge> &emap);

    bool buildMaxFlowSolutionFromPaths(vector<vector<int>> &paths);

    static bool BFSGraph(int sid, int tid, int num_vertices, int **graph, int **res_graph, vector<int> &parent);
    static int findMaxflowByFordFulkerson(Bipartite &in_bipartite, vector<vector<int>> &paths, int l_limit, int r_limit);



    void clear();
};


#endif // __BIPARTITE_HH__