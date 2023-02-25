#ifndef __STRIPE_GROUP_HH__
#define __STRIPE_GROUP_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "ConvertibleCode.hh"
#include "ClusterSettings.hh"
#include "Stripe.hh"

enum TransApproach {
    RE_ENCODE,
    PARITY_MERGE
};

class StripeGroup
{
private:

    ConvertibleCode _code;
    ClusterSettings _settings;
    vector<Stripe *> _stripes;
    size_t _id;

public:
    StripeGroup(size_t id, ConvertibleCode &code, ClusterSettings &settings, vector<Stripe *> &stripes);
    ~StripeGroup();

    size_t getId();
    ConvertibleCode &getCode();
    ClusterSettings &getClusterSettings();
    vector<Stripe *> &getStripes();

    vector<size_t> getDataDistribution();
    vector<vector<size_t> > getParityDistributions();
    vector<size_t> getParityDistribution(size_t parity_id);

    int getMinTransitionCost();
    int getDataRelocationCost();
    int getMinParityMergingCost();
    int getMinReEncodingCost();

    void print();
};


#endif // __STRIPE_GROUP_HH__