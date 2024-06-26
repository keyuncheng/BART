#ifndef __STRIPE_HH__
#define __STRIPE_HH__

#include "../include/include.hh"
#include "ConvertibleCode.hh"
#include "ClusterSettings.hh"
#include "../util/Utils.hh"

class Stripe
{
private:
public:
    u16string indices; // stripe length: n
    uint32_t id;

    Stripe();
    ~Stripe();

    /**
     * @brief print stripe
     *
     */
    void print();
};

#endif // __STRIPE_HH__