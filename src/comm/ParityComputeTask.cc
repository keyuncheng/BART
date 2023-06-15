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

ParityComputeTask::ParityComputeTask(ConvertibleCode *_code, uint32_t _post_stripe_id, uint8_t _post_block_id, uint16_t _src_node_id, EncodeMethod _enc_method, vector<uint16_t> &_src_block_nodes, vector<uint16_t> &_parity_reloc_nodes, string &_parity_block_src_path, string &_raw_dst_block_paths) : code(_code), post_stripe_id(_post_stripe_id), post_block_id(_post_block_id), src_node_id(_src_node_id), enc_method(_enc_method), src_block_nodes(_src_block_nodes), parity_reloc_nodes(_parity_reloc_nodes), parity_block_src_path(_parity_block_src_path)
{
    // skip instructions
    if (_post_stripe_id == INVALID_STRIPE_ID_GLOBAL)
    {
        return;
    }

    // process the paths
    if (enc_method == EncodeMethod::RE_ENCODE)
    {
        parity_block_dst_paths.resize(code->m_f);

        // parity paths
        string delimiter_char = ":";
        size_t pos = 0;
        string token;
        uint8_t parity_id = 0;

        while ((pos = _raw_dst_block_paths.find(delimiter_char)) != std::string::npos)
        {
            token = _raw_dst_block_paths.substr(0, pos);
            parity_block_dst_paths[parity_id] = token;
            _raw_dst_block_paths.erase(0, pos + delimiter_char.length());
            parity_id++;
        }
        parity_block_dst_paths[code->m_f - 1] = _raw_dst_block_paths;
    }
    else if (enc_method == EncodeMethod::PARITY_MERGE)
    {
        parity_block_dst_paths.resize(1);
        // pre_stripe_id
        string delimiter_char = ":";
        size_t pos = _raw_dst_block_paths.find(delimiter_char);
        string token = _raw_dst_block_paths.substr(0, pos);
        _raw_dst_block_paths.erase(0, pos + delimiter_char.length());

        // pre-stripe-id
        pre_stripe_id = atoi(token.c_str());

        // parity block path
        parity_block_dst_paths[0] = _raw_dst_block_paths;
    }

    // init all_tasks_finished (false)
    all_tasks_finished = false;
}

ParityComputeTask::~ParityComputeTask()
{
}

void ParityComputeTask::print()
{
    printf("\nParityComputeTask enc_method: %u, pre_stripe_id: %u, post-stripe: (%u, %u), src_node_id: %u\n", enc_method, pre_stripe_id, post_stripe_id, post_block_id, src_node_id);

    // printf("src_block_nodes:\n");
    // Utils::printVector(src_block_nodes);

    // printf("parity_reloc_nodes\n");
    // Utils::printVector(parity_reloc_nodes);

    // printf("parity_block_src_path: %s\n", parity_block_src_path.c_str());

    // printf("parity_block_dst_path(s):\n");
    // for (auto path : parity_block_dst_paths)
    // {
    //     printf("%s\n", path.c_str());
    // }
    // printf("\n\n");
}