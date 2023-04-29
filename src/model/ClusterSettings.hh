#ifndef __CLUSTER_SETTINGS_HH__
#define __CLUSTER_SETTINGS_HH__

typedef struct ClusterSettings
{
    uint16_t M; // number of storage nodes
    size_t N;   // number of stripes

    void print()
    {
        printf("ClusterSettings: num nodes: %u, num_stripes: %lu\n", M, N);
    }
} ClusterSettings;

#endif // __CLUSTER_SETTINGS_HH__