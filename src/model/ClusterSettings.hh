#ifndef __CLUSTER_SETTINGS_HH__
#define __CLUSTER_SETTINGS_HH__

typedef struct ClusterSettings {
    size_t M; // number of storage nodes
    size_t N; // number of stripes
} ClusterSettings;

#endif // __CLUSTER_SETTINGS_HH__