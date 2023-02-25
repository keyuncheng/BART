#include "Stripe.hh"

Stripe::Stripe(size_t id, vector<size_t> &stripe_indices)
{
    _id = id;
    _stripe_indices = stripe_indices;
}

Stripe::~Stripe()
{
}

vector<size_t> &Stripe::getStripeIndices() {
    return _stripe_indices;
}

size_t Stripe::getId() {
    return _id;

}

void Stripe::print() {
    for (auto idx : _stripe_indices) {
        printf("%ld ", idx);
    }
    printf("\n");
}
