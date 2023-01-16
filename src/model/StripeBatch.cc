#include "StripeBatch.hh"

StripeBatch::StripeBatch(int k_old, int m_old, int k_new, int m_new, vector<StripeGroup> stripe_groups)
{
    _k_old = k_old;
    _m_old = m_old;
    _n_old = _k_old + _m_old;

    _k_new = k_new;
    _m_new = m_new;
    _n_new = _k_new + _m_new;
    
    _stripe_groups = stripe_groups;
}

StripeBatch::~StripeBatch()
{
}
