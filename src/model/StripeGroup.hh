#ifndef __STRIPE_GROUP_HH__
#define __STRIPE_GROUP_HH__

#include "../include/include.hh"
#include "ConvertibleCode.hh"
#include "ClusterSettings.hh"
#include "Stripe.hh"

class StripeGroup
{
private:

    ConvertibleCode _code;
    ClusterSettings _settings;
    vector<Stripe> _stripes;
    int _id;

public:
    StripeGroup(ConvertibleCode code, ClusterSettings settings, int id, vector<Stripe> stripes);
    ~StripeGroup();

    ConvertibleCode &getCode();
    ClusterSettings &getClusterSettings();
    vector<Stripe> &getStripes();
    int getId();

    void print();
};


#endif // __STRIPE_GROUP_HH__