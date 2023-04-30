#include "Stripe.hh"

Stripe::Stripe()
{
}

Stripe::~Stripe()
{
}
void Stripe::print()
{
    printf("Stripe %u, indices: ", id);
    Utils::printVector(indices);
}
