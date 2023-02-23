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
    int _id;

public:
    StripeGroup(int id, ConvertibleCode &code, ClusterSettings &settings, vector<Stripe *> &stripes);
    ~StripeGroup();

    int getId();
    ConvertibleCode &getCode();
    ClusterSettings &getClusterSettings();
    vector<Stripe *> &getStripes();

    vector<int> getDataDistribution();
    vector<vector<int> > getParityDistributions();
    vector<int> getParityDistribution(int parity_id);

    int getMinTransitionCost();
    int getDataRelocationCost();
    int getMinParityMergingCost();
    int getMinReEncodingCost();

    void print();
};


#endif // __STRIPE_GROUP_HH__