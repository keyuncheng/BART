#include "model/Stripe.hh"
#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "model/StripeBatch.hh"
#include "model/RandomSolution.hh"
#include "model/BWOptSolution.hh"
#include "model/BART.hh"

int main(int argc, char *argv[])
{

    if (argc != 11)
    {
        printf("usage: ./BTSGenerator k_i m_i k_f m_f num_nodes num_stripes approach[RDRE/RDPM/BWRE/BWPM/BTRE/BTPM/BT] pre_placement_filename post_placement_filename sg_meta_filename");
        return -1;
    }

    uint8_t k_i = atoi(argv[1]);
    uint8_t m_i = atoi(argv[2]);
    uint8_t k_f = atoi(argv[3]);
    uint8_t m_f = atoi(argv[4]);
    uint16_t num_nodes = atoi(argv[5]);
    uint32_t num_stripes = atoi(argv[6]);
    string approach = argv[7];
    string pre_placement_filename = argv[8];
    string post_placement_filename = argv[9];
    string sg_meta_filename = argv[10];

    // random generator
    mt19937 random_generator = Utils::createRandomGenerator();

    // initialize code
    ConvertibleCode code(k_i, m_i, k_f, m_f);
    ClusterSettings settings(num_nodes, num_stripes);
    code.print();
    settings.print();

    // check the number of stripes are valid
    if (code.isValidForPM() == false || settings.isParamValid(code) == false)
    {
        printf("invalid parameters\n");
        return -1;
    }

    // Stripe batch
    StripeBatch stripe_batch(0, code, settings, random_generator);

    // stripe generator
    StripeGenerator stripe_generator;

    // load pre-transition stripes from placement file
    stripe_generator.loadStripes(code.n_i, pre_placement_filename, stripe_batch.pre_stripes);

    // printf("stripes:\n");
    // for (auto &stripe : stripe_batch.pre_stripes)
    // {
    //     stripe.print();
    // }

    // solutions and load distribution
    TransSolution trans_solution(code, settings);

    // benchmarking
    double finish_time = 0.0;
    struct timeval start_time, end_time;
    gettimeofday(&start_time, nullptr);

    if (approach == "RDRE" || approach == "RDPM")
    {
        RandomSolution random_solution(random_generator);
        random_solution.genSolution(stripe_batch, approach);
    }
    else if (approach == "BWRE" || approach == "BWPM")
    {
        // BWOptSolution
        BWOptSolution stripe_merge(random_generator);
        stripe_merge.genSolution(stripe_batch, approach);
    }
    else if (approach == "BTRE" || approach == "BTPM" || approach == "BT")
    {
        // BART solution
        BART bart(random_generator);
        bart.genSolution(stripe_batch, approach);
    }
    else
    {
        printf("invalid approach: %s\n", approach.c_str());
        return -1;
    }

    gettimeofday(&end_time, nullptr);
    finish_time = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000;
    printf("finished generating solution, time: %f ms\n", finish_time);

    // validate block placement
    if (trans_solution.isFinalBlockPlacementValid(stripe_batch) == false)
    {
        printf("error: invalid final block placements!\n");
        return -1;
    }

    // build transition tasks
    trans_solution.buildTransTasks(stripe_batch);
    // trans_solution.print();

    // store post-transition stripes to placement file
    stripe_generator.storeStripes(stripe_batch.post_stripes, post_placement_filename);

    // store stripe group metadata to sg_meta_filename metadata file
    stripe_batch.storeSGMetadata(sg_meta_filename);

    // get load distribution
    vector<u32string> transfer_load_dist = trans_solution.getTransferLoadDist();

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
    double percent_imbalance = (max_load - mean_send_load) / mean_send_load * 100;

    printf("================ Approach : %s =========================\n", approach.c_str());
    printf("send load: ");
    Utils::printVector(send_load_dist);
    printf("recv load: ");
    Utils::printVector(recv_load_dist);

    printf("send load: min: %u, max: %u, mean: %f, stddev: %f, cv: %f\n", min_send_load, max_send_load, mean_send_load, stddev_send_load, cv_send_load);

    printf("recv load: min: %u, max: %u, mean: %f, stddev: %f, cv: %f\n", min_recv_load, max_recv_load, mean_recv_load, stddev_recv_load, cv_recv_load);

    printf("max_load: %u, bandwidth: %lu, percent_imbalance: %f\n", max_load, total_bandwidth, percent_imbalance);

    return 0;
}