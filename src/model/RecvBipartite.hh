#ifndef __RECV_BIPARTITE_HH__
#define __RECV_BIPARTITE_HH__

#include "Bipartite.hh"
#include "../util/Utils.hh"

enum BlockType
{
    DATA_BLK,       // data block
    PARITY_BLK,     // parity block
    COMPUTE_BLK_RE, // compute block for re-encoding
    COMPUTE_BLK_PM, // compute block for parity merging
    INVALID_BLK
};

typedef struct BlockMeta
{
    size_t id;
    BlockType type;          // block type
    size_t stripe_batch_id;  // stripe batch id
    size_t stripe_group_id;  // stripe group id
    size_t stripe_id_global; // global stripe id (among all stripes)
    size_t stripe_id;        // stripe id in stripe_group_id [0, lambda_i)
    size_t block_id;         // block id in the stripe
    size_t vtx_id;           // Vertex id in RecvBipartite

    BlockMeta() : id(INVALID_ID), type(BlockType::INVALID_BLK), stripe_batch_id(INVALID_ID), stripe_group_id(INVALID_ID), stripe_id_global(INVALID_ID), stripe_id(INVALID_ID), block_id(INVALID_ID), vtx_id(INVALID_ID){};
} BlockMeta;

class RecvBipartite : public Bipartite
{
private:
public:
    RecvBipartite();
    ~RecvBipartite();

    // block metadata
    void print_block_metastore();
    size_t createBlockMeta(BlockMeta &in_block_meta);
    size_t findBlockMeta(BlockMeta &in_block_meta);

    size_t createBlockVtx(BlockMeta &in_block_meta);
    size_t createNodeVtx(size_t node_id);

    // construct stripe batch with the given approaches
    bool constructStripeBatchWithApproaches(StripeBatch &stripe_batch, vector<size_t> &approaches);
    // construct stripe group with a given approach
    bool constructStripeGroupWithApproach(StripeGroup &stripe_group, size_t approach);
    // construct stripe group
    bool constructSGWithData(StripeGroup &stripe_group);
    bool constructSGWithParityMerging(StripeGroup &stripe_group);
    bool constructSGWithReEncoding(StripeGroup &stripe_group);
    bool constructSGWithParityRelocation(StripeGroup &stripe_group);

    // find load-balanced solution greedily for each block in each stripe group
    bool findEdgesWithApproachesGreedy(StripeBatch &stripe_batch, vector<size_t> &sol_edges, mt19937 &random_generator);

    bool findEdgesWithApproachesGreedySorted(StripeBatch &stripe_batch, vector<size_t> &sol_edges, mt19937 &random_generator);

    // construct partial transition solution from edges
    bool constructPartialSolutionFromEdges(StripeBatch &stripe_batch, vector<size_t> &sol_edges, vector<vector<size_t>> &partial_solutions);

    // construct solution based on recv graph with approaches
    bool updatePartialSolutionFromRecvGraph(StripeBatch &stripe_batch, vector<vector<size_t>> &partial_solutions, vector<vector<size_t>> &solutions);

private:
    // block metadata
    unordered_map<size_t, BlockMeta> block_metastore;
    unordered_map<size_t, vector<BlockMeta *>> sg_block_meta_map; // <stripe_group_id, <block metadata> >

    unordered_map<size_t, size_t> block_meta_to_vtx_map; // <block metadata, vertex>
    unordered_map<size_t, size_t> vtx_to_block_meta_map; // <vertex, block metadata>

    unordered_map<size_t, size_t> node_to_vtx_map; // <node_id, vertex>
    unordered_map<size_t, size_t> vtx_to_node_map; // <vertex, node_id>
};

#endif // __RECV_BIPARTITE_HH__