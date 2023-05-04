#ifndef __STRIPE_MERGE_G_HH__
#define __STRIPE_MERGE_G_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "StripeGroup.hh"
#include "TransSolution.hh"

class StripeMerge
{
private:
    /* data */
public:
    mt19937 &random_generator;

    StripeMerge(mt19937 &_random_generator);
    ~StripeMerge();

    void genTransSolution(StripeBatch &stripe_batch, TransSolution &trans_solution);
    void genTransSolution(StripeGroup &stripe_group, TransSolution &trans_solution);
};

#endif