#ifndef __STRIPE_GENERATOR_HH__
#define __STRIPE_GENERATOR_HH__

#include <fstream>
#include <sstream>

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

    vector<Stripe> generateStripes(ConvertibleCode &code, ClusterSettings &settings);

    void storeStripes(vector<Stripe> &stripes, string placement_file);
    bool loadStripes(ConvertibleCode &code, ClusterSettings &settings, vector<Stripe> &stripes, string placement_file);
};

#endif // __STRIPE_GENERATOR_HH__