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

    // transitioning tasks
    map<uint32_t, vector<TransTask *>> sg_tasks;

    // different types of transitioning tasks (read, write, delete, transfer, compute)
    map<uint32_t, vector<TransTask *>> sg_read_tasks;
    map<uint32_t, vector<TransTask *>> sg_write_tasks;
    map<uint32_t, vector<TransTask *>> sg_delete_tasks;
    map<uint32_t, vector<TransTask *>> sg_transfer_tasks;
    map<uint32_t, vector<TransTask *>> sg_compute_tasks;

    TransSolution(ConvertibleCode &_code, ClusterSettings &_settings);
    ~TransSolution();

    /**
     * @brief initialize task lists
     *
     */
    void init();

    /**
     * @brief destroy task lists
     *
     */
    void destroy();

    /**
     * @brief check if the block placements are valid (subject to fault tolerance) in the final stripes
     *
     * @param stripe_batch
     * @return true
     * @return false
     */
    bool isFinalBlockPlacementValid(StripeBatch &stripe_batch);

    /**
     * @brief build transitioning tasks based on the stripe group
     * constructions, encoding nodes and final block
     * stripe placements
     *
     * @param stripe_batch
     */
    void buildTransTasks(StripeBatch &stripe_batch);

    /**
     * @brief get the load distribution of the built transitioning tasks
     *
     * @return vector<u32string>
     */
    vector<u32string> getTransferLoadDist();

    /**
     * @brief print tasks
     *
     */
    void print();
};

#endif // __TRANS_SOLUTION_HH__