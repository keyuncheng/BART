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
    ConvertibleCode &code;

    // erasure coding
    uint32_t post_stripe_id; // post_stripe_id (stripe group id)
    uint8_t post_block_id;   // post block id
    EncodeMethod enc_method;
    unsigned char *buffer;

    // TODO: add block save path

    // collected counter
    uint8_t collected;

    // table
    unsigned char **encode_gftbl;

    unsigned char **data_buffer;
    unsigned char **parity_buffer;

    ParityComputeTask(ConvertibleCode &_code, uint32_t _post_stripe_id, uint8_t _post_block_id, unsigned char *_buffer);
    ~ParityComputeTask();
};

#endif // __PARITY_COMPUTE_TASK_HH__