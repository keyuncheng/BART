#include "ParityComputeTask.hh"

ParityComputeTask::ParityComputeTask(ConvertibleCode *_code, uint32_t _post_stripe_id, uint8_t _post_block_id, unsigned char *_buffer, string _raw_path) : code(_code), post_stripe_id(_post_stripe_id), post_block_id(_post_block_id), buffer(_buffer)
{
    if (_post_stripe_id == INVALID_STRIPE_ID || _post_block_id == INVALID_BLK_ID)
    {
        return;
    }

    // extract parity block paths
    char *raw_str = (char *)malloc(_raw_path.size() * sizeof(char));
    memcpy(raw_str, _raw_path.c_str(), _raw_path.size() * sizeof(char));

    // encode method
    if (post_block_id < code->k_f)
    {
        enc_method = EncodeMethod::RE_ENCODE;
        re_parity_paths.assign(code->m_f, string());

        // extract re-encoding: m parity block paths
        uint8_t pid = 0;
        char *token = strtok(raw_str, ":");
        re_parity_paths[pid] = token;
        pid++;

        while ((token = strtok(NULL, ":")))
        {
            re_parity_paths[pid] = token;
            pid++;
        }

        free(raw_str);
    }
    else
    {
        enc_method = EncodeMethod::PARITY_MERGE;

        // pre_stripe_id
        char *token = strtok(raw_str, ":");
        pre_stripe_id = atoi(token);

        // parity block path
        while ((token = strtok(NULL, ":")))
        {
            pm_parity_path = token;
        }
    }

    // init num of collected parity blocks
    collected = 0;
}

ParityComputeTask::~ParityComputeTask()
{
}

void ParityComputeTask::print()
{
    string block_path;
    if (enc_method == EncodeMethod::RE_ENCODE)
    {
        block_path = re_parity_paths[0];
    }
    else if (enc_method == EncodeMethod::PARITY_MERGE)
    {
        block_path = pm_parity_path;
    }
    printf("ParityComputeTask post-stripe: (%u, %u), enc_method: %u, pre_stripe_id: %u, parity_block_path: %s\n", post_stripe_id, post_block_id, enc_method, pre_stripe_id, block_path.c_str());
}