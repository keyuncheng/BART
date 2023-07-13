#ifndef __BW_OPT_SOLUTION_HH__
#define __BW_OPT_SOLUTION_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "StripeGroup.hh"

/***
 * Bandwidth optimized solution for transitioning
 */

class BWOptSolution
{
private:
    /* data */
public:
    mt19937 &random_generator;

    BWOptSolution(mt19937 &_random_generator);
    ~BWOptSolution();

    /**
     * @brief generate transitioning solution for the stripe batch
     *
     * @param stripe_batch
     * @param approach transitioning approach (re-encoding, parity merging)
     */
    void genSolution(StripeBatch &stripe_batch, string approach);

    /**
     * @brief generate transitioning solution for the stripe group
     *
     * @param stripe_group
     * @param approach transitioning approach (re-encoding, parity merging)
     */
    void genSolution(StripeGroup &stripe_group, string approach);
};

#endif // __BW_OPT_SOLUTION_HH__