#ifndef __RANDOM_SOLUTION_HH__
#define __RANDOM_SOLUTION_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "StripeGroup.hh"

class RandomSolution
{
private:
    /* data */
public:
    mt19937 &random_generator;

    RandomSolution(mt19937 &_random_generator);
    ~RandomSolution();

    void genSolution(StripeBatch &stripe_batch, string approach);
    void genSolution(StripeGroup &stripe_group, string approach);
};

#endif // __RANDOM_SOLUTION_HH__