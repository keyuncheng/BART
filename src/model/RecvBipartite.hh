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

    int get_node_vtx_id();
    void print_meta();

private:
    bool addStripeGroupWithData(StripeGroup &stripe_group);
    bool addStripeGroupWithParityMerging(StripeGroup &stripe_group);
    bool addStripeGroupWithReEncoding(StripeGroup &stripe_group);
    bool addStripeGroupWithPartialParityMerging(StripeGroup &stripe_group);


    Vertex *get_block_vtx(BlockMeta &in_block_meta);
    Vertex *get_node_vtx(NodeMeta &in_node_meta);


    map<int, Vertex> internal_vertices_map; // internal vertices map (for re-encoding only)

    map<int, BlockMeta> data_block_meta_map; // data block metadata
    map<int, BlockMeta> parity_block_meta_map; // parity block metadata
    map<int, BlockMeta> compute_block_meta_map; // compute block metadata
    map<int, BlockMeta> compute_node_meta_map; // compute node metadata
    map<int, NodeMeta> node_meta_map; // node metadata

};


#endif // __RECV_BIPARTITE_HH__