#ifndef __STRIPE_BATCH_HH__
#define __STRIPE_BATCH_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeGroup.hh"

class StripeBatch
{
private:
public:
    size_t id;
    ConvertibleCode &code;
    ClusterSettings &settings;
    vector<Stripe> &stripes;
    mt19937 &random_generator;

    StripeGroup *enumerated_stripe_groups; // enumreated stripe groups
    vector<StripeGroup *> stripe_groups;   // chosen stripe groups

    StripeBatch(size_t _id, ConvertibleCode &_code, ClusterSettings &_settings, mt19937 &_random_generator, vector<Stripe> &_stripes);
    ~StripeBatch();

    bool constructSGInSequence();
    bool constructSGByRandomPick();
    bool constructSGByCost();

    // bool constructByCostAndSendLoad(vector<Stripe> &stripes, mt19937 &random_generator);

    // bool constructBySendLoadAndCost(vector<Stripe> &stripes, mt19937 &random_generator);

    // bool constructBySendLoadAndCostv2(vector<Stripe> &stripes, mt19937 &random_generator);

    void print();
};

#endif // __STRIPE_BATCH_HH__