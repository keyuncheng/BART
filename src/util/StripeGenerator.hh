#ifndef __STRIPE_GENERATOR_HH__
#define __STRIPE_GENERATOR_HH__

#include <fstream>
#include <sstream>

#include "../include/include.hh"
#include "../model/ConvertibleCode.hh"
#include "../model/ClusterSettings.hh"
#include "../model/Stripe.hh"
#include "../util/Utils.hh"

class StripeGenerator
{
private:
public:

    StripeGenerator();
    StripeGenerator(mt19937 &random_generator);
    ~StripeGenerator();

    vector<Stripe> generateRandomStripes(ConvertibleCode &code, ClusterSettings &settings, mt19937 &random_generator);

    void storeStripes(vector<Stripe> &stripes, string placement_file);
    bool loadStripes(ConvertibleCode &code, ClusterSettings &settings, string placement_file, vector<Stripe> &stripes);
};

#endif // __STRIPE_GENERATOR_HH__