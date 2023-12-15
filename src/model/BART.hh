#ifndef __BART_HH__
#define __BART_HH__

#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "TransSolution.hh"
#include "Bipartite.hh"
// #include "RecvBipartite.hh"

class BART
{
private:
    /* data */

public:
    mt19937 &random_generator;

    BART(mt19937 &_random_generator);
    ~BART();

    /**
     * @brief generate transitioning solution for the stripe batch
     *
     * @param stripe_batch
     * @param approach transitioning approach (re-encoding, parity merging)
     */
    void genSolution(StripeBatch &stripe_batch, string approach);

    /**
     * @brief generate solution for parity block generation for the stripe
     * batch (with parity merging only)
     *
     * @param stripe_batch
     */
    void genParityGenerationForPM(StripeBatch &stripe_batch);

    /**
     * @brief generate weighted solution for parity block generation for the
     * stripe batch (with parity merging only)
     *
     * @param stripe_batch
     */
    void genWeightedParityGenerationForPM(StripeBatch &stripe_batch);

    /**
     * @brief initialize load table for parity generation (data + parity version)
     *
     * @param stripe_batch
     * @param cur_lt
     */
    void initLTForParityGeneration(StripeBatch &stripe_batch, LoadTable &cur_lt);

    /**
     * @brief initialize load table for parity generation (data version)
     *
     * @param stripe_batch
     * @param cur_lt
     */
    void initLTForParityGenerationData(StripeBatch &stripe_batch, LoadTable &cur_lt);

    /**
     * @brief initialize solution for parity generation (with parity
     * merging only) (data + parity version)
     *
     * @param stripe_batch
     * @param is_perfect_pm identify whether each of the parity blocks can be
     * perfectly merged
     * @param cur_lt current load table
     */
    void initSolOfParityGenerationForPM(StripeBatch &stripe_batch, vector<vector<bool>> &is_perfect_pm, LoadTable &cur_lt);

    /**
     * @brief initialize solution for parity generation (with parity
     * merging only) (data version)
     *
     * @param stripe_batch
     * @param is_perfect_pm identify whether each of the parity blocks can be
     * perfectly merged
     * @param cur_lt current load table
     */
    void initSolOfParityGenerationForPMData(StripeBatch &stripe_batch, vector<vector<bool>> &is_perfect_pm, LoadTable &cur_lt);

    /**
     * @brief optimize solution for parity generation (with parity
     * merging only)
     *
     * @param stripe_batch
     * @param is_perfect_pm identify whether each of the parity blocks can be
     * perfectly merged
     * @param cur_lt current load table
     */
    void optimizeSolOfParityGenerationForPM(StripeBatch &stripe_batch, vector<vector<bool>> &is_perfect_pm, LoadTable &cur_lt);

    /**
     * @brief generate solution for parity block generation for the stripe
     * batch (hybrid with re-encoding and parity merging)
     *
     * @param stripe_batch
     * @param approach
     */
    void genParityComputationHybrid(StripeBatch &stripe_batch, string approach);

    /**
     * @brief generate solution for stripe re-distribution for the stripe batch
     *
     * @param stripe_batch
     */
    void genStripeRedistribution(StripeBatch &stripe_batch);
};

#endif // __BART_HH__