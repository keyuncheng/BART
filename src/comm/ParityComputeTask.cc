#include "ParityComputeTask.hh"

ParityComputeTask::ParityComputeTask()
{
    code = NULL;
    post_stripe_id = INVALID_STRIPE_ID_GLOBAL;
    post_block_id = INVALID_BLK_ID;
    enc_method = EncodeMethod::UNKNOWN_METHOD;
    all_tasks_finished = false;
}

ParityComputeTask::ParityComputeTask(bool _all_tasks_finished) : all_tasks_finished(_all_tasks_finished)
{
}

ParityComputeTask::ParityComputeTask(ConvertibleCode *_code, uint32_t _post_stripe_id, uint8_t _post_block_id, EncodeMethod _enc_method, vector<uint16_t> &_src_block_nodes, vector<uint16_t> &_parity_reloc_nodes, string &_raw_src_block_paths, string &_raw_dst_block_paths) : code(_code), post_stripe_id(_post_stripe_id), post_block_id(_post_block_id), enc_method(_enc_method), src_block_nodes(_src_block_nodes), parity_reloc_nodes(_parity_reloc_nodes)
{
    string delimiter_char = ":";
    size_t pos = 0;
    string token;

    // process the paths
    if (enc_method == EncodeMethod::RE_ENCODE)
    {
        // src block paths
        src_block_paths.resize(code->k_f);
        pos = 0;
        uint8_t data_block_id = 0;
        while ((pos = _raw_src_block_paths.find(delimiter_char)) != std::string::npos)
        {
            token = _raw_src_block_paths.substr(0, pos);
            src_block_paths[data_block_id] = token;
            _raw_src_block_paths.erase(0, pos + delimiter_char.length());
            data_block_id++;
        }
        src_block_paths[code->k_f - 1] = _raw_src_block_paths;

        // dst block paths
        dst_block_paths.resize(code->m_f);
        pos = 0;
        uint8_t parity_id = 0;

        while ((pos = _raw_dst_block_paths.find(delimiter_char)) != std::string::npos)
        {
            token = _raw_dst_block_paths.substr(0, pos);
            dst_block_paths[parity_id] = token;
            _raw_dst_block_paths.erase(0, pos + delimiter_char.length());
            parity_id++;
        }
        dst_block_paths[code->m_f - 1] = _raw_dst_block_paths;
    }
    else if (enc_method == EncodeMethod::PARITY_MERGE)
    {
        // src block paths
        src_block_paths.resize(code->lambda_i);
        pos = 0;
        uint8_t pre_stripe_id = 0;
        while ((pos = _raw_src_block_paths.find(delimiter_char)) != std::string::npos)
        {
            token = _raw_src_block_paths.substr(0, pos);
            src_block_paths[pre_stripe_id] = token;
            _raw_src_block_paths.erase(0, pos + delimiter_char.length());
            pre_stripe_id++;
        }
        src_block_paths[code->lambda_i - 1] = _raw_src_block_paths;

        // dst block paths
        dst_block_paths.resize(1);
        dst_block_paths[0] = _raw_dst_block_paths;
    }

    // init all_tasks_finished (false)
    all_tasks_finished = false;
}

ParityComputeTask::~ParityComputeTask()
{
}

void ParityComputeTask::print()
{
    printf("\nParityComputeTask post-stripe: (%u, %u), enc_method: %u\n", post_stripe_id, post_block_id, enc_method);

    printf("src_block_nodes:\n");
    Utils::printVector(src_block_nodes);

    printf("parity_reloc_nodes\n");
    Utils::printVector(parity_reloc_nodes);

    printf("src_block_paths:\n");
    for (auto path : src_block_paths)
    {
        printf("%s\n", path.c_str());
    }

    printf("dst_block_paths:\n");
    for (auto path : dst_block_paths)
    {
        printf("%s\n", path.c_str());
    }
    printf("\n\n");
}