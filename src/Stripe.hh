#include "include.hh"

class Stripe
{
private:
    int _k; // eck
    int _m; // ecm
    int _n; // ecn = eck + ecm

    int _M; // num of nodes

    vector<int> _stripe; // stripe length: n


public:
    Stripe(int k, int m, int M, vector<int> stripe);
    ~Stripe();
};

