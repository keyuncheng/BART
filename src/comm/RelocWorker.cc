#include "RelocWorker.hh"

RelocWorker::RelocWorker(Config &_config, unsigned int _self_worker_id, uint16_t _self_conn_id, MultiWriterQueue<Command> &_reloc_task_queue) : ThreadPool(1), config(_config), self_worker_id(_self_worker_id), self_conn_id(_self_conn_id), reloc_task_queue(_reloc_task_queue)
{
}

RelocWorker::~RelocWorker()
{
}

void RelocWorker::run()
{
    printf("[Node %u, Worker %u] RelocWorker::run start to handle relocation tasks\n", self_conn_id, self_worker_id);

    unsigned char *data_buffer = (unsigned char *)malloc(config.block_size * sizeof(unsigned char));

    unsigned int num_term_signals = 0;

    while (true)
    {
        if (finished() == true && reloc_task_queue.IsEmpty() == true)
        {
            break;
        }

        Command cmd_reloc;

        if (reloc_task_queue.Pop(cmd_reloc) == true)
        {
            // terminate signal
            if (cmd_reloc.type == CommandType::CMD_STOP)
            {
                num_term_signals++;

                // term signals (one from CmdHandler; anothers ares from ComputeWorker)
                if (num_term_signals == config.num_compute_workers + 1)
                {
                    setFinished();
                }

                continue;
            }

            if (cmd_reloc.type != CommandType::CMD_TRANSFER_RELOC_BLK)
            {
                fprintf(stderr, "RelocWorker::run error invalid command type: %u\n", cmd_reloc.type);
                exit(EXIT_FAILURE);
            }

            printf("[Node %u, Worker %u] RelocWorker::run received relocation task, post: (%u, %u)\n", self_conn_id, self_worker_id, cmd_reloc.post_stripe_id, cmd_reloc.post_block_id);

            // read block
            if (BlockIO::readBlock(cmd_reloc.src_block_path, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "RelocWorker::handleDataTransfer error reading block: %s\n", cmd_reloc.src_block_path.c_str());
                exit(EXIT_FAILURE);
            }

            // create connection to the block request handler
            string block_req_ip = config.agent_addr_map[cmd_reloc.dst_conn_id].first;
            unsigned int block_req_port = config.agent_addr_map[cmd_reloc.dst_conn_id].second + config.settings.num_nodes; // DEBUG

            sockpp::tcp_connector connector;
            while (!(connector = sockpp::tcp_connector(sockpp::inet_address(block_req_ip, block_req_port))))
            {
                this_thread::sleep_for(chrono::milliseconds(1));
            }

            if (connector.write_n(cmd_reloc.content, MAX_CMD_LEN * sizeof(unsigned char)) == -1)
            {
                fprintf(stderr, "RelocWorker::run error sending cmd, type: %u, src_conn_id: %u, dst_conn_id: %u\n", cmd_reloc.type, cmd_reloc.src_conn_id, cmd_reloc.dst_conn_id);
                exit(EXIT_FAILURE);
            }

            // send block
            if (BlockIO::sendBlock(connector, data_buffer, config.block_size) != config.block_size)
            {
                fprintf(stderr, "RelocWorker::handleDataTransfer error sending block: %s to RelocWorker %u\n", cmd_reloc.dst_block_path.c_str(), cmd_reloc.src_conn_id);
                exit(EXIT_FAILURE);
            }

            connector.close();

            printf("[Node %u, Worker %u] RelocWorker::run finished relocation task, post: (%u, %u)\n", self_conn_id, self_worker_id, cmd_reloc.post_stripe_id, cmd_reloc.post_block_id);
        }
    }

    free(data_buffer);

    printf("[Node %u, Worker %u] RelocWorker::run finished handling relocation tasks\n", self_conn_id, self_worker_id);
}