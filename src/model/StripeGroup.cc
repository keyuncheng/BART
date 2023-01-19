#include "StripeGroup.hh"

StripeGroup::StripeGroup(ConvertibleCode code, ClusterSettings settings, int id, vector<Stripe> stripes)
{
    _code = code;
    _settings = settings;
    _id = id;
    _stripes = stripes;
}

StripeGroup::~StripeGroup()
{
}

ConvertibleCode &StripeGroup::getCode() {
    return _code;
}

ClusterSettings &StripeGroup::getClusterSettings() {
    return _settings;
}

vector<Stripe> &StripeGroup::getStripes() {
    return _stripes;
}

int StripeGroup::getId() {
    return _id;
}

void StripeGroup::print() {
    printf("Stripe group %d:\n", _id);
    for (auto &stripe : _stripes) {
        stripe.print();
    }
}