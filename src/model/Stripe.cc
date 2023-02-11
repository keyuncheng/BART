#include "Stripe.hh"

Stripe::Stripe(ConvertibleCode code, ClusterSettings settings, int id, vector<int> stripe_indices)
{
    _code = code;
    _settings = settings;
    _id = id;
    _stripe_indices = stripe_indices;
}

Stripe::~Stripe()
{
}

ConvertibleCode &Stripe::getCode() {
    return _code;
}

ClusterSettings &Stripe::getClusterSettings() {
    return _settings;
}

vector<int> &Stripe::getStripeIndices() {
    return _stripe_indices;
}

int Stripe::getId() {
    return _id;

}

void Stripe::print() {
    for (int i = 0; i < _code.n_i; i++) {
        printf("%d ", _stripe_indices[i]);
    }
    printf("\n");
}
