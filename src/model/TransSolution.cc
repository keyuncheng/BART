#include "TransSolution.hh"

TransSolution::TransSolution(ConvertibleCode &_code, ClusterSettings &_settings) : code(_code), settings(_settings)
{
    init();
}

TransSolution::~TransSolution()
{
    destroy();
}

void TransSolution::init()
{
    sg_tasks.clear();
    sg_transfer_tasks.clear();

    if (TRANSFER_TASKS_ONLY == false)
    {
        sg_read_tasks.clear();
        sg_write_tasks.clear();
        sg_delete_tasks.clear();
        sg_compute_tasks.clear();
    }
}

void TransSolution::destroy()
{
    for (auto &item : sg_tasks)
    {
        for (auto &task : item.second)
        {
            delete task;
        }
    }
}

void TransSolution::print()
{
    for (auto &item : sg_tasks)
    {
        printf("stripe group %u, number of tasks: %lu\n", item.first, item.second.size());
        for (auto &task : item.second)
        {
            task->print();
        }
    }
}

vector<u32string> TransSolution::getTransferLoadDist()
{
    // send and receive load distribution
    // idx 0: send; idx 1: receive
    vector<u32string> load_dist(2, u32string(settings.num_nodes, 0));

    uint32_t count = 0;
    for (auto &item : sg_transfer_tasks)
    {

        for (auto &task : item.second)
        {
            load_dist[0][task->node_id]++;
            load_dist[1][task->dst_node_id]++;
        }
    }

    return load_dist;
}