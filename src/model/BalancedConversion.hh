#ifndef __BALANCED_CONVERSION_HH__
#define __BALANCED_CONVERSION_HH__

#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "TransSolution.hh"
#include "Bipartite.hh"
// #include "RecvBipartite.hh"

class BalancedConversion
{
private:
    /* data */
    void genParityGenerationLTs(StripeBatch &stripe_batch);
    void genBlockRelocation(StripeBatch &stripe_batch, TransSolution &trans_solution);
    void buildTransTasks(StripeBatch &stripe_batch, unordered_map<uint32_t, vector<pair<uint8_t, uint16_t>>> sg_blocks_to_reloc, unordered_map<uint32_t, vector<uint16_t>> sg_final_block_placement, TransSolution &trans_solution);

public:
    mt19937 &random_generator;

    BalancedConversion(mt19937 &_random_generator);
    ~BalancedConversion();

    void genTransSolution(StripeBatch &stripe_batch, TransSolution &trans_solution);

    // void getSolutionForStripeBatchGlobal(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator);

    // void getSolutionForStripeBatchGreedy(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator);

    // void getSolutionForStripeBatchIter(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator);

    // void getSolutionForStripeBatchAssigned(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator);

    // void getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<size_t>> &solutions, vector<size_t> &send_load_dist, vector<size_t> &recv_load_dist);
};

#endif // __BALANCED_CONVERSION_HH__