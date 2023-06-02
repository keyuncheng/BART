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
    void genParityComputation(StripeBatch &stripe_batch, string approach);
    void genParityComputationOptimized(StripeBatch &stripe_batch, string approach);

    void genBlockRelocation(StripeBatch &stripe_batch);

public:
    mt19937 &random_generator;

    BalancedConversion(mt19937 &_random_generator);
    ~BalancedConversion();

    void genSolution(StripeBatch &stripe_batch, string approach);

    // void getSolutionForStripeBatchGlobal(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator);

    // void getSolutionForStripeBatchGreedy(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator);

    // void getSolutionForStripeBatchIter(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator);

    // void getSolutionForStripeBatchAssigned(StripeBatch &stripe_batch, vector<vector<size_t>> &solutions, mt19937 random_generator);

    // void getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<size_t>> &solutions, vector<size_t> &send_load_dist, vector<size_t> &recv_load_dist);
};

#endif // __BALANCED_CONVERSION_HH__