#ifndef __STRIPE_BATCH_HH__
#define __STRIPE_BATCH_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "StripeGroup.hh"

class StripeBatch
{
private:

    ConvertibleCode _code;
    ClusterSettings _settings;
    vector<StripeGroup> _stripe_groups;
    int _id;

public:
    StripeBatch(int id, ConvertibleCode &code, ClusterSettings &settings);
    ~StripeBatch();

    bool constructInSequence(vector<Stripe> &stripes);
    bool constructByRandomPick(vector<Stripe> &stripes, mt19937 &random_generator);
    bool constructByCost(vector<Stripe> &stripes);

    int getId();
    ConvertibleCode &getCode();
    ClusterSettings &getClusterSettings();
    vector<StripeGroup> &getStripeGroups();

    void print();
};


#endif // __STRIPE_BATCH_HH__