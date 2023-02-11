#ifndef __STRIPE_HH__
#define __STRIPE_HH__

#include "../include/include.hh"
#include "ConvertibleCode.hh"
#include "ClusterSettings.hh"

class Stripe
{
private:
    ConvertibleCode _code;
    ClusterSettings _settings;
    vector<int> _stripe_indices; // stripe length: n
    int _id;

public:

    Stripe(ConvertibleCode code, ClusterSettings settings, int id, vector<int> stripe_indices);
    ~Stripe();

    ConvertibleCode &getCode();
    ClusterSettings &getClusterSettings();
    vector<int> &getStripeIndices();
    int getId();
    void print();
};

#endif // __STRIPE_HH__