#ifndef __STRIPE_GENERATOR_HH__
#define __STRIPE_GENERATOR_HH__

#include "../include/include.hh"
#include "../model/Stripe.hh"

class StripeGenerator
{
private:
    /* data */
public:
    StripeGenerator(/* args */);
    ~StripeGenerator();

    vector<Stripe> GenerateStripes(int k, int m, int N, int M);
};

#endif // __STRIPE_GENERATOR_HH__