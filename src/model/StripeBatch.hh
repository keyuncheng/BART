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
    StripeBatch(ConvertibleCode code, ClusterSettings settings, int id);
    ~StripeBatch();

    ConvertibleCode &getCode();
    ClusterSettings &getClusterSettings();
    vector<StripeGroup> &getStripeGroups();
    int getId();

    void print();
};


#endif // __STRIPE_BATCH_HH__