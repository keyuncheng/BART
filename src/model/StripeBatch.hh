#ifndef __STRIPE_BATCH_HH__
#define __STRIPE_BATCH_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeGroup.hh"

class StripeBatch
{
private:
public:
    uint8_t id;
    ConvertibleCode &code;
    ClusterSettings &settings;
    mt19937 &random_generator;
    vector<Stripe> &sb_stripes;

    // step 1: stripe group selection
    map<uint32_t, StripeGroup> selected_sgs; // selected stripe groups in order <sg_id, StripeGroup>

    StripeBatch(uint8_t _id, ConvertibleCode &_code, ClusterSettings &_settings, mt19937 &_random_generator, vector<Stripe> &_sb_stripes);
    ~StripeBatch();

    void constructSGInSequence();
    void constructSGByRandomPick();
    void constructSGByCost();

    // bool constructByCostAndSendLoad(vector<Stripe> &stripes, mt19937 &random_generator);

    // bool constructBySendLoadAndCost(vector<Stripe> &stripes, mt19937 &random_generator);

    // bool constructBySendLoadAndCostv2(vector<Stripe> &stripes, mt19937 &random_generator);

    void print();
};

#endif // __STRIPE_BATCH_HH__