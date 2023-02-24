// #ifndef __STRIPE_META_HH__
// #define __STRIPE_META_HH__

// enum BlockType {
//     DATA_BLK,
//     PARITY_BLK,
//     COMPUTE_BLK, // dummy block for computation
//     COMPUTE_NODE, // dummy node for re-encoding
//     COMPUTE_BLK_RE, // compute block for re-encoding
//     COMPUTE_BLK_PM // compute block for parity merging
// };

// typedef struct BlockMeta {
//     int id; // meta id
//     BlockType type; // block type
//     int stripe_batch_id; // stripe batch id
//     int stripe_group_id; // stripe group id
//     int stripe_id_global; // stripe id (among all stripes)
//     int stripe_id; // stripe id (in the stripe group)
//     int block_id; // block id in the stripe (0 - k - 1)
//     int vtx_id; // Vertex id (in the graph)
// } BlockMeta;

// typedef struct NodeMeta {
//     int id; // meta id
//     int node_id; // node id
//     int vtx_id; // vertex id
// } NodeMeta;




// #endif // __STRIPE_META_HH__