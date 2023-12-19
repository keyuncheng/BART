#ifndef __STATS_HH__
#define __STATS_HH__

#include "../include/include.hh"
#include "../util/Utils.hh"
#include "../model/ClusterSettings.hh"

class Stats
{
private:
public:
    Stats(/* args */);
    ~Stats();

    /**
     * @brief print load stats
     *
     * @param transfer_load_dist transferred load distribution (send and receive)
     */
    static void printLoadStats(vector<u32string> &transfer_load_dist);

    /**
     * @brief print weighted load stats (weights = load / upload or download bw)
     *
     * @param transfer_load_dist transferred load distribution (send and receive)
     * @param settings cluster settings
     */
    static void printWeightedLoadStats(vector<u32string> &transfer_load_dist, ClusterSettings &settings);
};

#endif // __STATS_HH__