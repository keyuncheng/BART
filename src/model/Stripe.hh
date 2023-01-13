#ifndef __STRIPE_HH__
#define __STRIPE_HH__

#include "../include/include.hh"

class Stripe
{
private:
    int _k; // eck
    int _m; // ecm
    int _n; // ecn = eck + ecm

    int _M; // num of nodes

    vector<int> _stripe_indices; // stripe length: n


public:

    Stripe(int k, int m, int M, vector<int> stripe_indices);
    ~Stripe();

    void print();
};

#endif // __STRIPE_HH__