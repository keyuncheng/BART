#ifndef __TRANS_SOLUTION_HH__
#define __TRANS_SOLUTION_HH__

#include "../include/include.hh"
#include "ConvertibleCode.hh"
#include "ClusterSettings.hh"
#include "TransTask.hh"
#include "StripeBatch.hh"

#define TRANSFER_TASKS_ONLY false

class TransSolution
{
private:
    /* data */
public:
    ConvertibleCode &code;
    ClusterSettings &settings;

    map<uint32_t, vector<TransTask *>> sg_tasks;

    map<uint32_t, vector<TransTask *>> sg_read_tasks;
    map<uint32_t, vector<TransTask *>> sg_write_tasks;
    map<uint32_t, vector<TransTask *>> sg_delete_tasks;
    map<uint32_t, vector<TransTask *>> sg_transfer_tasks;
    map<uint32_t, vector<TransTask *>> sg_compute_tasks;

    TransSolution(ConvertibleCode &_code, ClusterSettings &_settings);
    ~TransSolution();

    void init();
    void destroy();

    bool isFinalBlockPlacementValid(StripeBatch &stripe_batch);
    void buildTransTasks(StripeBatch &stripe_batch);

    vector<u32string> getTransferLoadDist();

    void print();
};

#endif // __TRANS_SOLUTION_HH__