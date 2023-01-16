#ifndef __STRIPE_GENERATOR_HH__
#define __STRIPE_GENERATOR_HH__

#include "../include/include.hh"
#include "../model/ConvertibleCode.hh"
#include "../model/ClusterSettings.hh"
#include "../model/Stripe.hh"

class StripeGenerator
{
private:
    /* data */
public:
    StripeGenerator(/* args */);
    ~StripeGenerator();

    vector<Stripe> GenerateStripes(ConvertibleCode code, ClusterSettings settings);
};

#endif // __STRIPE_GENERATOR_HH__