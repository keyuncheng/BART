#ifndef __CLUSTER_SETTINGS_HH__
#define __CLUSTER_SETTINGS_HH__

typedef struct ClusterSettings {
    int M; // number of storage nodes
    int N; // number of stripes
} ClusterSettings;

#endif // __CLUSTER_SETTINGS_HH__