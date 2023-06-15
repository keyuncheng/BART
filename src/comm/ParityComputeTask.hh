#ifndef __PARITY_COMPUTE_TASK_HH__
#define __PARITY_COMPUTE_TASK_HH__

#include "../include/include.hh"
#include "../model/ConvertibleCode.hh"
#include "../model/StripeGroup.hh"

class ParityComputeTask
{
private:
    /* data */
public:
    ConvertibleCode *code;

    // erasure coding
    uint32_t post_stripe_id; // post_stripe_id (stripe group id)
    uint8_t post_block_id;   // post block id
    uint8_t pre_stripe_id;   // for parity merging only
    uint16_t src_node_id;    // source node id
    EncodeMethod enc_method; // encoding method

    vector<uint16_t> src_block_nodes; // source block nodes (for re-encoding, reloc_nodes stores code.k_f nodes; for parity merging, reloc_nodes stores code.lambda_i nodes)

    vector<uint16_t> parity_reloc_nodes; // parity relocation nodes (for re-encoding, reloc_nodes stores code.m_f nodes; for parity merging, reloc_nodes store the corresponding parity node only)

    string parity_block_src_path; // source block path

    // dst block paths (for re-encoding, reloc_nodes stores code.m_f nodes; for parity merging, reloc_nodes store the corresponding parity node only)
    vector<string>
        parity_block_dst_paths;

    // terminate signal
    bool all_tasks_finished;

    ParityComputeTask();
    ParityComputeTask(bool _all_tasks_finished);
    ParityComputeTask(ConvertibleCode *_code, uint32_t _post_stripe_id, uint8_t _post_block_id, uint16_t _src_node_id, EncodeMethod _enc_method, vector<uint16_t> &_src_block_nodes, vector<uint16_t> &_parity_reloc_nodes, string &_parity_block_src_path, string &_raw_dst_block_paths);
    ~ParityComputeTask();

    void print();
};

#endif // __PARITY_COMPUTE_TASK_HH__