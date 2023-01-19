#include "StripeBatch.hh"

StripeBatch::StripeBatch(ConvertibleCode code, ClusterSettings settings, int id)
{
    _code = code;
    _settings = settings;
    _id = id;
}

StripeBatch::~StripeBatch()
{
}

ConvertibleCode &StripeBatch::getCode() {
    return _code;
}

ClusterSettings &StripeBatch::getClusterSettings() {
    return _settings;
}

vector<StripeGroup> &StripeBatch::getStripeGroups() {
    return _stripe_groups;
}

int StripeBatch::getId() {
    return _id;
}

void StripeBatch::print() {
    printf("StripeBatch %d:\n", _id);
    for (auto &stripe_group : _stripe_groups) {
        stripe_group.print();
    }
}