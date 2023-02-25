#ifndef __STRIPE_HH__
#define __STRIPE_HH__

#include "../include/include.hh"
#include "ConvertibleCode.hh"
#include "ClusterSettings.hh"

class Stripe
{
private:
    vector<size_t> _stripe_indices; // stripe length: n
    size_t _id;

public:

    Stripe(size_t id, vector<size_t> &stripe_indices);
    ~Stripe();

    size_t getId();
    vector<size_t> &getStripeIndices();
    void print();
};

#endif // __STRIPE_HH__