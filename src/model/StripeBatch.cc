#include "StripeBatch.hh"

StripeBatch::StripeBatch(ConvertibleCode code, ClusterSettings settings, int id, vector<StripeGroup> stripe_groups)
{
    _code = code;
    _settings = settings;
    _id = id;
    _stripe_groups = stripe_groups;
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