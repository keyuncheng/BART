// #ifndef __RECV_BIPARTITE_HH__
// #define __RECV_BIPARTITE_HH__

// #include "Bipartite.hh"
// #include "StripeMeta.hh"
// #include "../util/Utils.hh"

// class RecvBipartite : public Bipartite
// {
// private:

// public:
//     RecvBipartite();
//     ~RecvBipartite();

//     bool addStripeBatch(StripeBatch &stripe_batch);
//     bool addStripeGroup(StripeGroup &stripe_group);

//     void print();
//     void print_meta();

//     bool BFSGraphForRecvGraph(int sid, int tid, int num_vertices, int **graph, int **res_graph, vector<int> &parent, unordered_map<int, vector<int>> &cur_reloc_node_map);
//     int findMaxflowByFordFulkersonForRecvGraph(int l_limit, int r_limit);
//     bool findNegativeCycle(int **res_graph_weight_mtx, int **res_graph_cost_mtx, int num_vertices, int src_idx, int sink_idx, vector<int> &nccycle, int l_limit, int r_limit);

//     Vertex *getBlockVtx(BlockMeta &in_block_meta);
//     Vertex *getNodeVtx(NodeMeta &in_node_meta);

//     BlockMeta *getBlockMeta(int vtx_id);
//     NodeMeta *getNodeMeta(int vtx_id);

//     void getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<int> &load_dist);

//     // construct stripe batch with approaches
//     bool constructStripeBatchWithApproaches(StripeBatch &stripe_batch, vector<int> &approaches);

//     // construct stripe group with approaches
//     bool constructStripeGroupWithApproach(StripeGroup &stripe_group, int approach);

//     bool constructSGWithData(StripeGroup &stripe_group);
//     bool constructSGWithParityMerging(StripeGroup &stripe_group);
//     bool constructSGWithReEncoding(StripeGroup &stripe_group);
//     bool constructSGWithParityRelocation(StripeGroup &stripe_group);

//     // find load-balanced solution greedily for each block in each stripe group
//     bool findEdgesWithApproachesGreedy(StripeBatch &stripe_batch, vector<int> &edges, mt19937 &random_generator);

//     // construct partial transition solution from edges
//     bool constructPartialSolutionFromEdges(StripeBatch &stripe_batch, vector<int> &edges, vector<vector<int> > &partial_solutions);

    

// private:
//     bool addStripeGroupWithData(StripeGroup &stripe_group);
//     bool addStripeGroupWithParityMerging(StripeGroup &stripe_group);
//     bool addStripeGroupWithReEncoding(StripeGroup &stripe_group);
//     bool addStripeGroupWithPartialParityMerging(StripeGroup &stripe_group);

//     unordered_map<int, Vertex *> internal_vertices_map; // internal vertices map (for re-encoding only)
    
//     unordered_map<int, BlockMeta> block_meta_map; // block meta map
//     unordered_map<int, NodeMeta> node_meta_map; // node metadata

//     unordered_map<int, BlockMeta *> data_block_meta_map; // data block metadata
//     unordered_map<int, BlockMeta *> parity_block_meta_map; // parity block metadata
//     unordered_map<int, BlockMeta *> compute_block_meta_map; // compute block metadata
//     unordered_map<int, BlockMeta *> compute_node_meta_map; // compute node metadata

//     unordered_map<int, int> block_meta_to_vtx_map;
//     unordered_map<int, int> node_meta_to_vtx_map;
//     unordered_map<int, int> vtx_to_block_meta_map;
//     unordered_map<int, int> vtx_to_node_meta_map;

// };


// #endif // __RECV_BIPARTITE_HH__