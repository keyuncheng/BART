#include "Stats.hh"

Stats::Stats(/* args */)
{
}

Stats::~Stats()
{
}

void Stats::printLoadStats(vector<u32string> &transfer_load_dist)
{
    u32string &send_load_dist = transfer_load_dist[0];
    u32string &recv_load_dist = transfer_load_dist[1];

    // send load
    // min, max
    uint32_t min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
    uint32_t max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
    // mean, stddev, cv
    double mean_send_load = 1.0 * std::accumulate(send_load_dist.begin(), send_load_dist.end(), 0) / send_load_dist.size();
    double sqr_send_load = 0;
    for (auto &item : send_load_dist)
    {
        sqr_send_load += pow((double)item - mean_send_load, 2);
    }
    double stddev_send_load = std::sqrt(sqr_send_load / send_load_dist.size());
    double cv_send_load = stddev_send_load / mean_send_load;

    // recv load
    // min max
    uint32_t min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
    uint32_t max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());
    // mean, stddev, cv
    double mean_recv_load = 1.0 * std::accumulate(recv_load_dist.begin(), recv_load_dist.end(), 0) / recv_load_dist.size();
    double sqr_recv_load = 0;
    for (auto &item : recv_load_dist)
    {
        sqr_recv_load += pow((double)item - mean_recv_load, 2);
    }
    double stddev_recv_load = std::sqrt(sqr_recv_load / recv_load_dist.size());
    double cv_recv_load = stddev_recv_load / mean_recv_load;

    // max load
    uint32_t max_load = max(max_send_load, max_recv_load);

    // bandwidth
    uint64_t total_bandwidth = 0;
    for (auto item : send_load_dist)
    {
        total_bandwidth += item;
    }

    // percent imbalance metric
    // double percent_imbalance = (max_load - mean_send_load) / mean_send_load * 100;
    printf("send load: ");
    Utils::printVector(send_load_dist);
    printf("recv load: ");
    Utils::printVector(recv_load_dist);

    printf("send load: min: %u, max: %u, mean: %f, stddev: %f, cv: %f\n", min_send_load, max_send_load, mean_send_load, stddev_send_load, cv_send_load);

    printf("recv load: min: %u, max: %u, mean: %f, stddev: %f, cv: %f\n", min_recv_load, max_recv_load, mean_recv_load, stddev_recv_load, cv_recv_load);

    printf("max_load: %u, bandwidth: %lu\n", max_load, total_bandwidth);
    // printf("max_load: %u, bandwidth: %lu, percent_imbalance: %f\n", max_load, total_bandwidth, percent_imbalance);
}

void Stats::printWeightedLoadStats(vector<u32string> &transfer_load_dist, ClusterSettings &settings)
{
    uint16_t num_nodes = settings.num_nodes;
    u32string &send_load_dist = transfer_load_dist[0];
    u32string &recv_load_dist = transfer_load_dist[1];

    // weighted load table
    vector<double> weighted_upload_dist(send_load_dist.size(), 0);
    vector<double> weighted_download_dist(recv_load_dist.size(), 0);

    for (uint16_t i = 0; i < num_nodes; i++)
    {
        weighted_upload_dist[i] = 1.0 * send_load_dist[i] / settings.bw_profile.upload[i];
        weighted_download_dist[i] = 1.0 * recv_load_dist[i] / settings.bw_profile.download[i];
    }

    // weighted send load
    // min, max
    double min_weighted_send_load = *min_element(weighted_upload_dist.begin(), weighted_upload_dist.end());
    double max_weighted_send_load = *max_element(weighted_upload_dist.begin(), weighted_upload_dist.end());
    // mean, stddev, cv
    double mean_send_load = 1.0 * std::accumulate(weighted_upload_dist.begin(), weighted_upload_dist.end(), 0) / weighted_upload_dist.size();
    double sqr_send_load = 0;
    for (auto &item : weighted_upload_dist)
    {
        sqr_send_load += pow((double)item - mean_send_load, 2);
    }
    double stddev_send_load = std::sqrt(sqr_send_load / weighted_upload_dist.size());
    double cv_send_load = stddev_send_load / mean_send_load;

    // weighted recv load
    // min max
    double min_weighted_recv_load = *min_element(weighted_download_dist.begin(), weighted_download_dist.end());
    double max_weighted_recv_load = *max_element(weighted_download_dist.begin(), weighted_download_dist.end());
    // mean, stddev, cv
    double mean_recv_load = 1.0 * std::accumulate(weighted_download_dist.begin(), weighted_download_dist.end(), 0) / weighted_download_dist.size();
    double sqr_recv_load = 0;
    for (auto &item : weighted_download_dist)
    {
        sqr_recv_load += pow((double)item - mean_recv_load, 2);
    }
    double stddev_recv_load = std::sqrt(sqr_recv_load / weighted_download_dist.size());
    double cv_recv_load = stddev_recv_load / mean_recv_load;

    // max weighted load
    double max_weighted_load = max(max_weighted_send_load, max_weighted_recv_load);

    // bandwidth
    uint64_t total_bandwidth = 0;
    for (auto item : send_load_dist)
    {
        total_bandwidth += item;
    }

    // percent imbalance metric
    // double percent_imbalance = (max_load - mean_send_load) / mean_send_load * 100;
    printf("send load: ");
    Utils::printVector(weighted_upload_dist);
    printf("recv load: ");
    Utils::printVector(weighted_download_dist);

    printf("send load: min: %.3f, max: %.3f, mean: %f, stddev: %f, cv: %f\n", min_weighted_send_load, max_weighted_send_load, mean_send_load, stddev_send_load, cv_send_load);

    printf("recv load: min: %.3f, max: %.3f, mean: %f, stddev: %f, cv: %f\n", min_weighted_recv_load, max_weighted_recv_load, mean_recv_load, stddev_recv_load, cv_recv_load);

    printf("max_weighted_load: %.3f, bandwidth: %lu\n", max_weighted_load, total_bandwidth);
    // printf("max_load: %u, bandwidth: %lu, percent_imbalance: %f\n", max_load, total_bandwidth, percent_imbalance);
}