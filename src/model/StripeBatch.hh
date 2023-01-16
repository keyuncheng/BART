#ifndef __STRIPE_BATCH_HH__
#define __STRIPE_BATCH_HH__

#include "../include/include.hh"
#include "StripeGroup.hh"

class StripeBatch
{
private:

    ConvertibleCode _code;
    ClusterSettings _settings;
    vector<StripeGroup> _stripe_groups;
    int _id;

public:
    StripeBatch(ConvertibleCode code, ClusterSettings settings, int id, vector<StripeGroup> stripe_groups);
    ~StripeBatch();

    ConvertibleCode &getCode();
    ClusterSettings &getClusterSettings();
    vector<StripeGroup> &getStripeGroups();
    int getId();
};


#endif // __STRIPE_BATCH_HH__