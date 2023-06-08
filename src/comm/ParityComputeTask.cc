#include "ParityComputeTask.hh"

ParityComputeTask::ParityComputeTask(ConvertibleCode &_code, uint32_t _post_stripe_id, uint8_t _post_block_id, unsigned char *_buffer) : code(_code), post_stripe_id(_post_stripe_id), post_block_id(_post_block_id), buffer(_buffer)
{
    // encode method
    if (post_block_id < code.k_f)
    {
        enc_method = EncodeMethod::RE_ENCODE;
    }
    else
    {
        enc_method = EncodeMethod::PARITY_MERGE;
    }

    collected = 0;
}

ParityComputeTask::~ParityComputeTask()
{
}