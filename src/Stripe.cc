#include "Stripe.hh"

Stripe::Stripe(int k, int m, int M, vector<int> stripe)
{
    _k = k;
    _m = m;
    _M = M;
    _stripe = stripe;
}

Stripe::~Stripe()
{
}
