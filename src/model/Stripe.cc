#include "Stripe.hh"

Stripe::Stripe(int id, vector<int> &stripe_indices)
{
    _id = id;
    _stripe_indices = stripe_indices;
}

Stripe::~Stripe()
{
}

vector<int> &Stripe::getStripeIndices() {
    return _stripe_indices;
}

int Stripe::getId() {
    return _id;

}

void Stripe::print() {
    for (auto idx : _stripe_indices) {
        printf("%d ", idx);
    }
    printf("\n");
}
