#include "Stripe.hh"

Stripe::Stripe()
{
}

Stripe::~Stripe()
{
}
void Stripe::print()
{
    printf("Stripe %ld, indices: ", id);
    Utils::printVector(indices);
    printf("\n");
}
