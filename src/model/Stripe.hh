#ifndef __STRIPE_HH__
#define __STRIPE_HH__

#include "../include/include.hh"
#include "ConvertibleCode.hh"
#include "ClusterSettings.hh"

class Stripe
{
private:
    vector<int> _stripe_indices; // stripe length: n
    int _id;

public:

    Stripe(int id, vector<int> &stripe_indices);
    ~Stripe();

    int getId();
    vector<int> &getStripeIndices();
    void print();
};

#endif // __STRIPE_HH__