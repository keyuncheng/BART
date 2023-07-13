#ifndef __BW_OPT_SOLUTION_HH__
#define __BW_OPT_SOLUTION_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "StripeGroup.hh"

class BWOptSolution
{
private:
    /* data */
public:
    mt19937 &random_generator;

    BWOptSolution(mt19937 &_random_generator);
    ~BWOptSolution();

    void genSolution(StripeBatch &stripe_batch, string approach);
    void genSolution(StripeGroup &stripe_group, string approach);
};

#endif // __BW_OPT_SOLUTION_HH__