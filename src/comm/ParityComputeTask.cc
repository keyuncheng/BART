#include "ParityComputeTask.hh"

ParityComputeTask::ParityComputeTask(ConvertibleCode *_code, uint32_t _post_stripe_id, uint8_t _post_block_id, EncodeMethod _enc_method, unsigned char *_buffer, string _raw_path) : code(_code), post_stripe_id(_post_stripe_id), post_block_id(_post_block_id), enc_method(_enc_method), buffer(_buffer)
{
    re_parity_paths.assign(code->m_f, string());

    if (_post_stripe_id == INVALID_STRIPE_ID_GLOBAL || _post_block_id == INVALID_BLK_ID)
    {
        return;
    }

    // process the paths
    if (enc_method == EncodeMethod::RE_ENCODE)
    {
        // parity paths
        string delimiter_char = ":";
        size_t pos = 0;
        string token;
        uint8_t parity_id = 0;

        while ((pos = _raw_path.find(delimiter_char)) != std::string::npos)
        {
            token = _raw_path.substr(0, pos);
            re_parity_paths[parity_id] = token;
            _raw_path.erase(0, pos + delimiter_char.length());
            parity_id++;
        }
        re_parity_paths[code->m_f - 1] = _raw_path;
    }
    else if (enc_method == EncodeMethod::PARITY_MERGE)
    {
        // pre_stripe_id
        string delimiter_char = ":";
        size_t pos = _raw_path.find(delimiter_char);
        string token = _raw_path.substr(0, pos);
        _raw_path.erase(0, pos + delimiter_char.length());

        // pre-stripe-id
        pre_stripe_id = atoi(token.c_str());

        // parity block path
        pm_parity_path = _raw_path;
    }

    // init num of collected parity blocks
    collected = 0;

    // init all_tasks_finished (false)
    all_tasks_finished = false;
}

ParityComputeTask::~ParityComputeTask()
{
}

void ParityComputeTask::print()
{
    printf("ParityComputeTask post-stripe: (%u, %u), enc_method: %u, pre_stripe_id: %u, parity_block_path(s):\n", post_stripe_id, post_block_id, enc_method, pre_stripe_id);

    if (enc_method == EncodeMethod::RE_ENCODE)
    {
        for (auto path : re_parity_paths)
        {
            printf("%s\n", path.c_str());
        }
    }
    else if (enc_method == EncodeMethod::PARITY_MERGE)
    {
        printf("%s\n", pm_parity_path.c_str());
    }
}