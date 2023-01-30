#ifndef __RECV_BIPARTITE_HH__
#define __RECV_BIPARTITE_HH__

#include "Bipartite.hh"
#include "StripeMeta.hh"
#include "../util/Utils.hh"

class RecvBipartite : public Bipartite
{
private:

public:
    RecvBipartite();
    ~RecvBipartite();

    bool addStripeBatch(StripeBatch &stripe_batch);
    bool addStripeGroup(StripeGroup &stripe_group);

    void print();
    void print_meta();

    bool BFSGraphForRecvGraph(int sid, int tid, int num_vertices, int **graph, int **res_graph, vector<int> &parent, map<int, vector<int>> &cur_reloc_node_map);
    int findMaxflowByFordFulkersonForRecvGraph(int l_limit, int r_limit);
    bool findNegativeCycle(int **res_graph_weight_mtx, int **res_graph_cost_mtx, int num_vertices, int src_idx, int sink_idx, vector<int> &nccycle, int l_limit, int r_limit);

    

private:
    bool addStripeGroupWithData(StripeGroup &stripe_group);
    bool addStripeGroupWithParityMerging(StripeGroup &stripe_group);
    bool addStripeGroupWithReEncoding(StripeGroup &stripe_group);
    bool addStripeGroupWithPartialParityMerging(StripeGroup &stripe_group);

    Vertex *getBlockVtx(BlockMeta &in_block_meta);
    Vertex *getNodeVtx(NodeMeta &in_node_meta);

    BlockMeta *getBlockMeta(int vtx_id);
    NodeMeta *getNodeMeta(int vtx_id);


    map<int, Vertex *> internal_vertices_map; // internal vertices map (for re-encoding only)
    
    map<int, BlockMeta> block_meta_map; // block meta map
    map<int, NodeMeta> node_meta_map; // node metadata

    map<int, BlockMeta *> data_block_meta_map; // data block metadata
    map<int, BlockMeta *> parity_block_meta_map; // parity block metadata
    map<int, BlockMeta *> compute_block_meta_map; // compute block metadata
    map<int, BlockMeta *> compute_node_meta_map; // compute node metadata

};


#endif // __RECV_BIPARTITE_HH__