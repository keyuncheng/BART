#ifndef __STRIPE_BATCH_HH__
#define __STRIPE_BATCH_HH__

#include "../include/include.hh"
#include "StripeGroup.hh"

class StripeBatch
{
private:

    int _k_old;
    int _m_old;
    int _n_old;

    int _k_new;
    int _m_new;
    int _n_new;

    vector<StripeGroup> _stripe_groups;
    /* data */
public:
    StripeBatch(int k_old, int m_old, int k_new, int m_new, vector<StripeGroup> stripe_groups);
    ~StripeBatch();
};


#endif // __STRIPE_BATCH_HH__