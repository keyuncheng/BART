#include "ComputeWorker.hh"

ComputeWorker::ComputeWorker(Config &_config, MessageQueue<ParityComputeTask> &_parity_compute_queue, MemoryPool &_memory_pool, unsigned _num_threads) : ThreadPool(_num_threads), config(_config), parity_compute_queue(_parity_compute_queue), memory_pool(_memory_pool)
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

    while (finished() == false || parity_compute_queue.IsEmpty() == false || ongoing_task_map.size() > 0)
    {
        ParityComputeTask parity_compute_task(&config.code, INVALID_STRIPE_ID, INVALID_BLK_ID, NULL, string());

        if (parity_compute_queue.Pop(parity_compute_task) == true)
        {
            // lock the map
            unique_lock<mutex> lck(ongoing_task_map_mtx);

            // generate parity compute key
            string parity_compute_key;
            if (parity_compute_task.enc_method == EncodeMethod::RE_ENCODE)
            { // Re-encoding format: post_stripe_id:enc_method
                parity_compute_key = to_string(parity_compute_task.post_stripe_id) + string(":") + to_string(parity_compute_task.enc_method);
            }
            else if (parity_compute_task.enc_method == EncodeMethod::PARITY_MERGE)
            { // Parity merging format: post_stripe_id:enc_method:post_block_id
                parity_compute_key = to_string(parity_compute_task.post_stripe_id) + string(":") + to_string(parity_compute_task.enc_method) + string(":") + to_string(parity_compute_task.post_block_id);
            }

            printf("ComputeWorker::run received parity computation task: %s\n", parity_compute_key.c_str());

            parity_compute_task.print();

            // check whether there is already a parity compute task
            auto it = ongoing_task_map.find(parity_compute_key);
            if (it == ongoing_task_map.end())
            { // add a new parity compute task
                ongoing_task_map.insert(pair<string, ParityComputeTask>(parity_compute_key, parity_compute_task));

                // obtain the task
                ParityComputeTask &task = ongoing_task_map.at(parity_compute_key);

                // allocate data buffer
                if (task.enc_method == EncodeMethod::RE_ENCODE)
                {
                    // allocate data block buffers (size: code.k_f (data blocks), passed from parity_compute_queue)
                    task.data_buffer = (unsigned char **)calloc(code.k_f, sizeof(unsigned char *));

                    // allocate parity block buffers (size: code.m_f (parity blocks))
                    task.parity_buffer = (unsigned char **)calloc(code.m_f, sizeof(unsigned char *));
                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        unsigned char *parity_block_buffer = memory_pool.getBlock();
                        task.parity_buffer[parity_id] = parity_block_buffer;
                    }
                }
                else if (task.enc_method == EncodeMethod::PARITY_MERGE)
                {
                    // allocate data buffer (size: code.lambda_i (corresponding parity blocks), passed from parity compute queue)
                    task.data_buffer = (unsigned char **)calloc(code.lambda_i, sizeof(unsigned char *));

                    // allocate parity block buffers (size: 1 corresponding parity block)
                    task.parity_buffer = (unsigned char **)calloc(1, sizeof(unsigned char *));
                    unsigned char *parity_block_buffer = memory_pool.getBlock();
                    task.parity_buffer[0] = parity_block_buffer;
                }

                printf("ComputeWorker::run allocated blocks for parity computation task: %s\n", parity_compute_key.c_str());
            }

            // process the parity block
            ParityComputeTask &ongoing_task = ongoing_task_map.at(parity_compute_key);

            // compare the path: whether it's correct
            if (ongoing_task.enc_method == EncodeMethod::RE_ENCODE)
            {
                if (ongoing_task.re_parity_paths[0].compare(parity_compute_task.re_parity_paths[0]))
                {
                    fprintf(stderr, "error: invalid parity block paths for re-encoding %s\n", parity_compute_task.re_parity_paths[0].c_str());
                    exit(EXIT_FAILURE);
                }
            }
            else if (ongoing_task.enc_method == EncodeMethod::PARITY_MERGE)
            {
                if (ongoing_task.pm_parity_path.compare(parity_compute_task.pm_parity_path))
                {
                    fprintf(stderr, "error: invalid parity block paths for parity merging %s\n", parity_compute_task.pm_parity_path.c_str());
                    exit(EXIT_FAILURE);
                }
            }

            // assign the buffer to ongoing task
            if (ongoing_task.enc_method == EncodeMethod::RE_ENCODE)
            {
                ongoing_task.data_buffer[parity_compute_task.post_block_id] = parity_compute_task.buffer;
            }
            else if (parity_compute_task.enc_method == EncodeMethod::PARITY_MERGE)
            { // assign the pre_stripe_id
                ongoing_task.data_buffer[parity_compute_task.pre_stripe_id] = parity_compute_task.buffer;
            }

            // add the collected block
            ongoing_task.collected++;

            printf("ComputeWorker::run collected block for parity computation task: %s, collected: %u\n", parity_compute_key.c_str(), ongoing_task.collected);

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
                            fprintf(stderr, "error: invalid data buffer for re-encoding: %u\n", data_block_id);
                            exit(EXIT_FAILURE);
                        }
                    }

                    // encode data
                    ec_encode_data(config.block_size, code.k_f, code.m_f, re_encode_gftbl, ongoing_task.data_buffer, ongoing_task.parity_buffer);

                    // write parity_block to disk
                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        // write parity_id to disk
                        BlockIO::writeBlock(ongoing_task.re_parity_paths[parity_id], ongoing_task.parity_buffer[parity_id], config.block_size);
                    }

                    // free blocks
                    for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                    {
                        memory_pool.freeBlock(ongoing_task.data_buffer[data_block_id]);
                    }

                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        memory_pool.freeBlock(ongoing_task.parity_buffer[parity_id]);
                    }

                    free(ongoing_task.data_buffer);
                    free(ongoing_task.parity_buffer);

                    // remove key
                    ongoing_task_map.erase(parity_compute_key);

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
                    ec_encode_data(config.block_size, code.lambda_i, 1, pm_encode_gftbl[ongoing_task.post_block_id - code.k_f], ongoing_task.data_buffer, ongoing_task.parity_buffer);

                    // write the parity block to disk
                    BlockIO::writeBlock(ongoing_task.pm_parity_path, ongoing_task.parity_buffer[0], config.block_size);

                    // free block
                    for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                    {
                        memory_pool.freeBlock(ongoing_task.data_buffer[pre_stripe_id]);
                    }

                    memory_pool.freeBlock(ongoing_task.parity_buffer[0]);

                    free(ongoing_task.data_buffer);
                    free(ongoing_task.parity_buffer);

                    // remove key
                    ongoing_task_map.erase(parity_compute_key);

                    printf("ComputeWorker::run performed parity merging for parity computation task: %s\n", parity_compute_key.c_str());
                }
            }
        }
    }

    printf("ComputeWorker::run finished handling parity computation tasks\n");
}