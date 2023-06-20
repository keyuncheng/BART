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
    EncodeMethod enc_method; // encoding method

    // source block nodes (for re-encoding, src_block_nodes stores code.k_f nodes; for parity merging, src_block_nodes stores code.lambda_i nodes)
    vector<uint16_t> src_block_nodes;

    // block reloc nodes (for re-encoding, parity_reloc_nodes stores code.m_f nodes; for parity merging, parity_reloc_nodes stores corresponding node only)
    vector<uint16_t> parity_reloc_nodes;

    // source block paths (for re-encoding, src_block_paths stores code.k_f paths; for parity merging, src_block_paths store code.lambda_i paths)
    vector<string> src_block_paths;

    // dst block paths (for re-encoding, dst_block_paths stores code.m_f paths; for parity merging, dst_block_paths store the path for corresponding parity block only)
    vector<string> dst_block_paths;

    // terminate signal
    bool all_tasks_finished;

    ParityComputeTask();
    ParityComputeTask(bool _all_tasks_finished);
    ParityComputeTask(ConvertibleCode *_code, uint32_t _post_stripe_id, uint8_t _post_block_id, EncodeMethod _enc_method, vector<uint16_t> &_src_block_nodes, vector<uint16_t> &_parity_reloc_nodes, string &_raw_src_block_paths, string &_raw_dst_block_paths);
    ~ParityComputeTask();

    void print();
};

#endif // __PARITY_COMPUTE_TASK_HH__