#ifndef __COMPUTE_TASK_HH__
#define __COMPUTE_TASK_HH__

#include "../include/include.hh"
#include "../model/ConvertibleCode.hh"

class ParityComputeTask
{
private:
    /* data */
public:
    ConvertibleCode &code;

    // erasure coding

    // table
    unsigned char **encode_gftbl;

    unsigned char **data_buffer;
    unsigned char **parity_buffer;

    ParityComputeTask(ConvertibleCode &_code);
    ~ParityComputeTask();

    void computeReencode(uint8_t parity_block_id, uint8_t data_block_id, unsigned char *buffer);
    void computeParityMerging(uint8_t parity_block_id, uint8_t data_block_id, unsigned char *buffer);
};

#endif // __COMPUTE_TASK_HH__