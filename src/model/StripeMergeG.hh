#ifndef __STRIPE_MERGE_G_HH__
#define __STRIPE_MERGE_G_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeBatch.hh"
#include "StripeGroup.hh"
#include "TransSolution.hh"

class StripeMergeG
{
private:
    /* data */
public:
    StripeMergeG();
    ~StripeMergeG();

    void genTransSolution(StripeBatch &stripe_batch, TransSolution &solution, mt19937 random_generator);
    void genTransSolution(StripeGroup &stripe_group, TransSolution &solution, mt19937 random_generator);
};

#endif