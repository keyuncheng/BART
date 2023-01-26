#ifndef __STRIPE_META_HH__
#define __STRIPE_META_HH__

enum BlockType {
    DATA_BLK,
    PARITY_BLK,
    COMPUTE_BLK, // dummy vertex for re-encoding
};

typedef struct BlockMeta {
    int id; // meta id
    int stripe_batch_id; // stripe batch id
    int stripe_group_id; // stripe group id
    int stripe_id; // stripe id (in the stripe group)
    int stripe_id_global; // stripe id (among all stripes)
    int block_id; // block id in the stripe (0 - k - 1)
    BlockType type; // block type
    int vtx_id; // Vertex id (in the graph)
} BlockMeta;

typedef struct NodeMeta {
    int id; // meta id
    int node_id; // node id
    int vtx_id; // vertex id
} NodeMeta;




#endif // __STRIPE_META_HH__