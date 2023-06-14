#include "ComputeWorker.hh"

ComputeWorker::ComputeWorker(Config &_config, MultiWriterQueue<ParityComputeTask> &_parity_compute_queue, MessageQueue<string> &_parity_comp_result_queue, MemoryPool &_memory_pool, unsigned _num_threads) : ThreadPool(_num_threads), config(_config), parity_compute_queue(_parity_compute_queue), parity_comp_result_queue(_parity_comp_result_queue), memory_pool(_memory_pool)
{
    // initialize EC tables (for both re-encoding and parity merging)
    initECTables();
}

ComputeWorker::~ComputeWorker()
{
    destroyECTables();
}

void ComputeWorker::initECTables()
{
    ConvertibleCode &code = config.code;

    // init re-encoding table
    re_matrix = (unsigned char *)malloc(code.n_f * code.k_f * sizeof(unsigned char));
    re_encode_gftbl = (unsigned char *)malloc(32 * code.k_f * code.m_f * sizeof(unsigned char));
    gf_gen_rs_matrix(re_matrix, code.n_f, code.k_f); // Vandermonde matrix
    ec_init_tables(code.k_f, code.m_f, &re_matrix[code.k_f * code.k_f], re_encode_gftbl);

    // init parity merging table
    pm_matrix = (unsigned char **)malloc(code.m_f * sizeof(unsigned char *));
    pm_encode_gftbl = (unsigned char **)malloc(code.m_f * sizeof(unsigned char *));
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        pm_matrix[parity_id] = (unsigned char *)malloc(code.lambda_i * sizeof(unsigned char));

        pm_encode_gftbl[parity_id] = (unsigned char *)malloc(32 * code.lambda_i * sizeof(unsigned char));

        pm_matrix[parity_id][0] = 1; // the first element is 1
        // the following elements are parity_id ^ (k_i * pre_stripe_id)
        // for example: (3,2) -> (9,2)
        // parity generation matrix:
        // 1^0 1^(3*1) 1^(3*2)
        // 2^0 2^(3*1) 2^(3*2)
        for (uint8_t pre_stripe_id = 1; pre_stripe_id < code.lambda_i; pre_stripe_id++)
        {
            pm_matrix[parity_id][pre_stripe_id] = gfPow(parity_id + 1, code.k_i * pre_stripe_id);
        }

        ec_init_tables(code.lambda_i, 1, pm_matrix[parity_id], pm_encode_gftbl[parity_id]);
    }
}

void ComputeWorker::destroyECTables()
{
    ConvertibleCode &code = config.code;

    // free merging tables
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        free(pm_encode_gftbl[parity_id]);
        free(pm_matrix[parity_id]);
    }
    free(pm_encode_gftbl);
    free(pm_matrix);

    // free re-encoding table
    free(re_encode_gftbl);
    free(re_matrix);
}

unsigned char ComputeWorker::gfPow(unsigned char val, unsigned int times)
{
    unsigned char ret_val = 1;

    for (unsigned int i = 0; i < times; i++)
    {
        ret_val = gf_mul(ret_val, val);
    }

    return ret_val;
}

void ComputeWorker::run()
{
    printf("ComputeWorker::run start to handle parity computation tasks\n");

    ConvertibleCode &code = config.code;

    // parity buffer for re-encoding and parity merging
    unsigned char **re_parity_buffer;
    unsigned char **pm_parity_buffer;

    // allocate parity block buffers for re-encoding (size: code.m_f (parity blocks))
    re_parity_buffer = (unsigned char **)calloc(code.m_f, sizeof(unsigned char *));
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        re_parity_buffer[parity_id] = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));
    }

    // allocate parity block buffers (size: 1 corresponding parity block)
    pm_parity_buffer = (unsigned char **)calloc(1, sizeof(unsigned char *));
    pm_parity_buffer[0] = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

    // allocate parity block buffers for parity merging
    ParityComputeTask parity_compute_task(&config.code, INVALID_STRIPE_ID, INVALID_BLK_ID, EncodeMethod::UNKNOWN_METHOD, NULL, string());

    while (true)
    {
        if (finished() == true)
        {
            if (parity_compute_queue.IsEmpty() == true && ongoing_task_map.size() == 0)
            {
                string term_signal = parity_comp_term_signal;
                parity_comp_result_queue.Push(term_signal);
                break;
            }
        }

        if (parity_compute_queue.Pop(parity_compute_task) == true)
        {
            if (parity_compute_task.all_tasks_finished == true)
            {
                setFinished();
                continue;
            }

            string parity_compute_key = getParityComputeTaskKey(parity_compute_task.enc_method, parity_compute_task.post_stripe_id, parity_compute_task.post_block_id);

            printf("ComputeWorker::run received parity computation task: %s, buffer: %p\n", parity_compute_key.c_str(), parity_compute_task.buffer);
            parity_compute_task.print();

            if (ongoing_task_map.find(parity_compute_key) == ongoing_task_map.end())
            { // (a compute task without data) add a new parity compute task
                ongoing_task_map.insert(pair<string, ParityComputeTask>(parity_compute_key, parity_compute_task));

                // obtain the task
                ParityComputeTask &task = ongoing_task_map.at(parity_compute_key);

                // allocate data buffer
                if (task.enc_method == EncodeMethod::RE_ENCODE)
                {
                    // allocate data block buffers (size: code.k_f (data blocks), passed from parity_compute_queue)
                    task.data_buffer = (unsigned char **)calloc(code.k_f, sizeof(unsigned char *));
                }
                else if (task.enc_method == EncodeMethod::PARITY_MERGE)
                {
                    // allocate data buffer (size: code.lambda_i (corresponding parity blocks), passed from parity compute queue)
                    task.data_buffer = (unsigned char **)calloc(code.lambda_i, sizeof(unsigned char *));
                }
            }

            // process the ongoing task for the parity block
            ParityComputeTask &ongoing_task = ongoing_task_map.at(parity_compute_key);

            if (parity_compute_task.buffer != NULL)
            {
                // assign the buffer to ongoing task
                if (ongoing_task.enc_method == EncodeMethod::RE_ENCODE)
                {
                    ongoing_task.re_parity_paths = parity_compute_task.re_parity_paths;
                    ongoing_task.data_buffer[parity_compute_task.post_block_id] = parity_compute_task.buffer;
                }
                else if (parity_compute_task.enc_method == EncodeMethod::PARITY_MERGE)
                { // assign the pre_stripe_id
                    ongoing_task.pm_parity_path = parity_compute_task.pm_parity_path;
                    ongoing_task.data_buffer[parity_compute_task.pre_stripe_id] = parity_compute_task.buffer;
                }

                // add the collected block
                ongoing_task.collected++;

                printf("ComputeWorker::run collected block for parity computation task: %s, collected: %u\n", parity_compute_key.c_str(), ongoing_task.collected);
            }

            // perform computation
            if (ongoing_task.enc_method == EncodeMethod::RE_ENCODE)
            { // compute re-encoding
                if (ongoing_task.collected == code.k_f)
                {
                    // check the buffers
                    for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                    {
                        if (ongoing_task.data_buffer[data_block_id] == NULL)
                        {
                            fprintf(stderr, "ComputeWorker::run error: invalid data buffer for re-encoding: %u\n", data_block_id);
                            exit(EXIT_FAILURE);
                        }
                    }

                    // encode data
                    ec_encode_data(config.block_size, code.k_f, code.m_f, re_encode_gftbl, ongoing_task.data_buffer, re_parity_buffer);

                    // write parity_block to disk
                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        // write parity_id to disk
                        if (BlockIO::writeBlock(ongoing_task.re_parity_paths[parity_id], re_parity_buffer[parity_id], config.block_size) != config.block_size)
                        {
                            fprintf(stderr, "ComputeWorker::run error writing block: %s\n", ongoing_task.re_parity_paths[parity_id].c_str());
                            exit(EXIT_FAILURE);
                        }

                        printf("ComputeWorker::run finished writing parity block %s\n", ongoing_task.re_parity_paths[parity_id].c_str());
                    }

                    // free blocks
                    for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                    {
                        // memory_pool.freeBlock(ongoing_task.data_buffer[data_block_id]);

                        free(ongoing_task.data_buffer[data_block_id]);
                    }

                    // remove ongoing task
                    ongoing_task_map.erase(parity_compute_key);

                    // push finished task
                    parity_comp_result_queue.Push(parity_compute_key);

                    printf("ComputeWorker::run performed re-encoding for parity computation task: %s\n", parity_compute_key.c_str());
                }
            }
            else if (ongoing_task.enc_method == EncodeMethod::PARITY_MERGE)
            {
                if (ongoing_task.collected == code.lambda_i)
                {
                    // check the buffers
                    for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                    {
                        if (ongoing_task.data_buffer[pre_stripe_id] == NULL)
                        {
                            fprintf(stderr, "error: invalid data buffer for parity merging: %u\n", pre_stripe_id);
                            exit(EXIT_FAILURE);
                        }
                    }

                    // compute parity merging
                    ec_encode_data(config.block_size, code.lambda_i, 1, pm_encode_gftbl[ongoing_task.post_block_id - code.k_f], ongoing_task.data_buffer, pm_parity_buffer);

                    // write the parity block to disk
                    if (BlockIO::writeBlock(ongoing_task.pm_parity_path, pm_parity_buffer[0], config.block_size) != config.block_size)
                    {
                        fprintf(stderr, "error writing block: %s\n", ongoing_task.pm_parity_path.c_str());
                        exit(EXIT_FAILURE);
                    }

                    printf("ComputeWorker::run finished writing parity block %s\n", ongoing_task.pm_parity_path.c_str());

                    // free block
                    for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                    {
                        // memory_pool.freeBlock(ongoing_task.data_buffer[pre_stripe_id]);

                        free(ongoing_task.data_buffer[pre_stripe_id]);
                    }

                    // remove ongoing task
                    ongoing_task_map.erase(parity_compute_key);

                    // push finished task
                    parity_comp_result_queue.Push(parity_compute_key);

                    printf("ComputeWorker::run performed parity merging for parity computation task: %s\n", parity_compute_key.c_str());
                }
            }
        }
    }

    // free buffers
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        free(re_parity_buffer[parity_id]);
    }
    free(pm_parity_buffer[0]);

    free(re_parity_buffer);
    free(pm_parity_buffer);

    printf("ComputeWorker::run finished handling parity computation tasks\n");
}

string ComputeWorker::getParityComputeTaskKey(EncodeMethod enc_method, uint32_t post_stripe_id, uint8_t post_block_id)
{
    string delimiter_char = ":";

    // generate parity compute key
    string parity_compute_key = to_string(post_stripe_id) + delimiter_char + to_string(enc_method);

    if (enc_method == EncodeMethod::PARITY_MERGE)
    { // Parity merging format: post_stripe_id:enc_method:post_block_id
        parity_compute_key = parity_compute_key + to_string(enc_method) + delimiter_char + to_string(post_block_id);
    }
    else if (enc_method == EncodeMethod::UNKNOWN_METHOD)
    {
        fprintf(stderr, "error: undefined parity generation key!\n");
        exit(EXIT_FAILURE);
    }

    return parity_compute_key;
}

void ComputeWorker::getParityComputeTaskFromKey(string key, EncodeMethod &enc_method, uint32_t &post_stripe_id, uint8_t &post_block_id)
{
    if (key.size() == 0)
    {
        fprintf(stderr, "ComputeWorker::getParityComputeTaskFromKey invalid key: %s\n", key.c_str());
        exit(EXIT_FAILURE);
    }

    string delimiter_char = ":";
    size_t pos = 0;
    pos = key.find(delimiter_char);
    post_stripe_id = stoul(key.substr(0, pos)); // get post_stripe_id
    key.erase(0, pos + delimiter_char.length());

    // check whether it's re-encoding or parity merging
    pos = key.find(delimiter_char);
    if (pos == string::npos)
    {
        enc_method = EncodeMethod::RE_ENCODE;
        post_block_id = INVALID_BLK_ID;
    }
    else
    {
        enc_method = EncodeMethod::PARITY_MERGE;
        post_block_id = atol(key.substr(pos + 1).c_str());
    }
}