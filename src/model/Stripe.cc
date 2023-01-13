#include "Stripe.hh"

Stripe::Stripe(int k, int m, int M, vector<int> stripe_indices)
{
    _k = k;
    _m = m;
    _n = _k + _m;
    _M = M;
    _stripe_indices = stripe_indices;
}

Stripe::~Stripe()
{
}

void Stripe::print() {
    for (int i = 0; i < _n; i++) {
        printf("%d ", _stripe_indices[i]);
    }
    printf("\n");
}