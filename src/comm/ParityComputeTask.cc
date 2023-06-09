#include "ParityComputeTask.hh"

ParityComputeTask::ParityComputeTask(ConvertibleCode *_code, uint32_t _post_stripe_id, uint8_t _post_block_id, unsigned char *_buffer, string _raw_path) : code(_code), post_stripe_id(_post_stripe_id), post_block_id(_post_block_id), buffer(_buffer)
{
    // encode method
    if (post_block_id < code->k_f)
    {
        enc_method = EncodeMethod::RE_ENCODE;
        re_parity_paths.assign(code->m_f, string());

        // extract re-encoding parity block paths
        char *raw_str = (char *)malloc(_raw_path.size() * sizeof(char));
        memcpy(raw_str, _raw_path.c_str(), _raw_path.size() * sizeof(char));

        uint8_t pid = 0;
        char *token = strtok(raw_str, ",");
        re_parity_paths[pid] = token;
        pid++;

        while ((token = strtok(NULL, ",")))
        {
            re_parity_paths[pid] = token;
            pid++;
        }

        free(raw_str);
    }
    else
    {
        enc_method = EncodeMethod::PARITY_MERGE;
        pm_parity_path = _raw_path;
    }

    collected = 0;
}

ParityComputeTask::~ParityComputeTask()
{
}