#include "StripeGroup.hh"

StripeGroup::StripeGroup(int k_old, int m_old, int k_new, int m_new, vector<Stripe> stripes)
{
    _k_old = k_old;
    _m_old = m_old;
    _n_old = _k_old + _m_old;

    _k_new = k_new;
    _m_new = m_new;
    _n_new = _k_new + _m_new;
    
    _stripes = stripes;
}

StripeGroup::~StripeGroup()
{
}
