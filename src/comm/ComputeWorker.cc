#include "ComputeWorker.hh"

ComputeWorker::ComputeWorker(Config &_config, MessageQueue<ParityComputeTask> &_parity_compute_queue, MemoryPool &_memory_pool) : config(_config), parity_compute_queue(_parity_compute_queue), memory_pool(_memory_pool)
{
}

ComputeWorker::~ComputeWorker()
{
}

void ComputeWorker::run()
{
    ConvertibleCode &code = config.code;

    while (finished() == false || parity_compute_queue.IsEmpty() == false)
    {
        ParityComputeTask parity_compute_task(config.code, INVALID_STRIPE_ID, INVALID_BLK_ID, NULL);

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
                    task.data_buffer = (unsigned char **)malloc(code.k_f * sizeof(unsigned char *));
                    task.parity_buffer = (unsigned char **)malloc(code.m_f * sizeof(unsigned char *));

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
                    task.data_buffer = (unsigned char **)malloc(code.lambda_i * sizeof(unsigned char *));
                    task.parity_buffer = (unsigned char **)malloc(1 * sizeof(unsigned char *));

                    unsigned char *parity_block_buffer = memory_pool.getBlock();
                    task.parity_buffer[0] = parity_block_buffer;

                    // assign the buffer
                    task.data_buffer[task.post_block_id - code.k_f] = task.buffer;

                    // add the collected block
                    task.collected++;
                }
            }

            // TODO:
            // process the parity block
            ParityComputeTask &task = ongoing_task_map.at(parity_compute_key);

            if (task.enc_method == EncodeMethod::RE_ENCODE)
            {
                if (task.collected == code.k_f)
                {
                    // compute re-encoding
                    // write to disk

                    for (uint8_t parity_id = 0; parity_id < code.m_f; parity_id++)
                    {
                        // write parity_id to disk
                    }
                }

                // free block
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

                    // write to disk
                }

                // free block
                memory_pool.freeBlock(task.parity_buffer[0]);
                free(task.parity_buffer);
                free(task.data_buffer);
            }
        }
    }
}