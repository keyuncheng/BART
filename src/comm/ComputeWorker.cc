#include "ComputeWorker.hh"

ComputeWorker::ComputeWorker(Config &_config, MessageQueue<ParityComputeTask> &_parity_compute_queue, MemoryPool &_memory_pool) : config(_config), parity_compute_queue(_parity_compute_queue), memory_pool(_memory_pool)
{
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
    gf_gen_rs_matrix(re_matrix, code.n_f, code.k_f);
    ec_init_tables(code.k_f, code.m_f, &re_matrix[code.k_f * code.k_f], re_encode_gftbl);

    // init parity merging table
    pm_matrix = (unsigned char **)malloc(code.m_f * sizeof(unsigned char *));
    pm_encode_gftbl = (unsigned char **)malloc(code.m_f * sizeof(unsigned char *));
    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        pm_matrix[parity_id] = (unsigned char *)malloc((code.lambda_i + 1) * code.lambda_i * sizeof(unsigned char));

        pm_encode_gftbl[parity_id] = (unsigned char *)malloc(32 * code.lambda_i * 1 * sizeof(unsigned char));

        for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
        {
            pm_matrix[parity_id][pre_stripe_id] = gfPow(pre_stripe_id + 1, code.k_i);
        }

        ec_init_tables(code.lambda_i, 1, &pm_matrix[parity_id][0], pm_encode_gftbl[parity_id]);
    }
}

void ComputeWorker::destroyECTables()
{
    ConvertibleCode &code = config.code;

    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
    {
        free(pm_encode_gftbl[parity_id]);
        free(pm_matrix[parity_id]);
    }
    free(pm_encode_gftbl);
    free(pm_matrix);

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
    ConvertibleCode &code = config.code;

    while (finished() == false || parity_compute_queue.IsEmpty() == false)
    {
        ParityComputeTask parity_compute_task(&config.code, INVALID_STRIPE_ID, INVALID_BLK_ID, NULL, string());

        if (parity_compute_queue.Pop(parity_compute_task) == true)
        {
            string parity_compute_key = to_string(parity_compute_task.post_stripe_id) + string(":") + to_string(parity_compute_task.enc_method) + to_string(parity_compute_task.post_block_id);

            unique_lock<mutex> lck(ongoing_task_map_mtx);

            auto it = ongoing_task_map.find(parity_compute_key);

            if (it == ongoing_task_map.end())
            {
                // add a parity compute task
                ongoing_task_map.insert(pair<string, ParityComputeTask>(parity_compute_key, parity_compute_task));

                ParityComputeTask &task = ongoing_task_map.at(parity_compute_key);

                // allocate data buffer
                if (task.enc_method == EncodeMethod::RE_ENCODE)
                {
                    // allocate data block buffer
                    task.data_buffer = (unsigned char **)calloc(code.k_f, sizeof(unsigned char *));
                    task.parity_buffer = (unsigned char **)calloc(code.m_f, sizeof(unsigned char *));

                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        unsigned char *parity_block_buffer = memory_pool.getBlock();
                        task.parity_buffer[parity_id] = parity_block_buffer;
                    }

                    // assign the buffer
                    task.data_buffer[task.post_block_id] = task.buffer;

                    // add the collected block
                    task.collected++;
                }
                else if (task.enc_method == EncodeMethod::PARITY_MERGE)
                {
                    // allocate parity block buffer
                    task.data_buffer = (unsigned char **)calloc(code.lambda_i, sizeof(unsigned char *));
                    task.parity_buffer = (unsigned char **)calloc(1, sizeof(unsigned char *));

                    unsigned char *parity_block_buffer = memory_pool.getBlock();
                    task.parity_buffer[0] = parity_block_buffer;

                    // assign the buffer
                    task.data_buffer[task.post_block_id - code.k_f] = task.buffer;

                    // add the collected block
                    task.collected++;
                }
            }

            // process the parity block
            ParityComputeTask &task = ongoing_task_map.at(parity_compute_key);

            if (task.enc_method == EncodeMethod::RE_ENCODE)
            {
                if (task.collected == code.k_f)
                {
                    // compute re-encoding
                    // check the buffers
                    for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                    {
                        if (task.data_buffer[data_block_id] == NULL)
                        {
                            fprintf(stderr, "error: invalid data buffer %u\n", data_block_id);
                            return;
                        }
                    }

                    ec_encode_data(config.block_size, code.k_f, code.m_f, re_encode_gftbl, task.data_buffer, task.parity_buffer);

                    // write parity_block to disk
                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        // write parity_id to disk
                        BlockIO::writeBlock(task.re_parity_paths[parity_id], task.parity_buffer[parity_id], config.block_size);
                    }
                }

                // free block
                for (uint8_t data_block_id = 0; data_block_id < code.k_f; data_block_id++)
                {
                    memory_pool.freeBlock(task.data_buffer[data_block_id]);
                }

                for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                {
                    memory_pool.freeBlock(task.parity_buffer[parity_id]);
                }

                free(task.parity_buffer);
                free(task.data_buffer);
            }
            else if (task.enc_method == EncodeMethod::PARITY_MERGE)
            {

                if (task.collected == code.lambda_i)
                {
                    // compute parity merging
                    ec_encode_data(config.block_size, code.lambda_i, 1, pm_encode_gftbl[task.post_block_id - code.k_f], task.data_buffer, task.parity_buffer);

                    // write the parity block to disk
                    BlockIO::writeBlock(task.pm_parity_path, task.parity_buffer[0], config.block_size);
                }

                // free block
                for (uint8_t pre_stripe_id = 0; pre_stripe_id < code.lambda_i; pre_stripe_id++)
                {
                    memory_pool.freeBlock(task.data_buffer[pre_stripe_id]);
                }

                memory_pool.freeBlock(task.parity_buffer[0]);
                free(task.parity_buffer);
                free(task.data_buffer);
            }
        }
    }
}