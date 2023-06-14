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
    EncodeMethod enc_method;
    unsigned char *buffer;

    vector<string> re_parity_paths;
    string pm_parity_path;

    // collected counter
    uint8_t collected;

    unsigned char **data_buffer;
    // unsigned char **parity_buffer;

    bool all_tasks_finished;

    ParityComputeTask(ConvertibleCode *_code, uint32_t _post_stripe_id, uint8_t _post_block_id, EncodeMethod _enc_method, unsigned char *_buffer, string _raw_path);
    ~ParityComputeTask();

    void print();
};

#endif // __PARITY_COMPUTE_TASK_HH__