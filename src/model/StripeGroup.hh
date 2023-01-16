#ifndef __STRIPE_GROUP_HH__
#define __STRIPE_GROUP_HH__

#include "../include/include.hh"
#include "Stripe.hh"

class StripeGroup
{
private:

    int _k_old;
    int _m_old;
    int _n_old;

    int _k_new;
    int _m_new;
    int _n_new;

    vector<Stripe> _stripes;
    /* data */
public:
    StripeGroup(int k_old, int m_old, int k_new, int m_new, vector<Stripe> stripes);
    ~StripeGroup();
};


#endif // __STRIPE_GROUP_HH__