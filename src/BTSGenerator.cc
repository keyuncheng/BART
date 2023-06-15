#include "model/Stripe.hh"
#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "model/StripeBatch.hh"
#include "model/RandomSolution.hh"
#include "model/StripeMerge.hh"
#include "model/BalancedConversion.hh"

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
        // StripeMerge
        StripeMerge stripe_merge(random_generator);
        stripe_merge.genSolution(stripe_batch, approach);
    }
    else if (approach == "BTRE" || approach == "BTPM" || approach == "BT")
    {
        // Balanced Conversion
        BalancedConversion balanced_conversion(random_generator);
        balanced_conversion.genSolution(stripe_batch, approach);
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
    trans_solution.print();

    // store post-transition stripes to placement file
    stripe_generator.storeStripes(stripe_batch.post_stripes, post_placement_filename);

    // store stripe group metadata to sg_meta_filename metadata file
    stripe_batch.storeSGMetadata(sg_meta_filename);

    // get load distribution
    vector<u32string> transfer_load_dist = trans_solution.getTransferLoadDist();

    u32string &send_load_dist = transfer_load_dist[0];
    u32string &recv_load_dist = transfer_load_dist[1];

    // get bandwidth
    uint64_t total_bandwidth = 0;
    for (auto item : send_load_dist)
    {
        total_bandwidth += item;
    }

    // in-degree
    // min, max
    uint32_t min_in_degree = *min_element(send_load_dist.begin(), send_load_dist.end());
    uint32_t max_in_degree = *max_element(send_load_dist.begin(), send_load_dist.end());
    // mean, stddev, cv
    double mean_in_degree = 1.0 * std::accumulate(send_load_dist.begin(), send_load_dist.end(), 0) / send_load_dist.size();
    double sq_sum_in_degree = std::inner_product(send_load_dist.begin(), send_load_dist.end(), send_load_dist.begin(), 0.0);
    double stddev_in_degree = std::sqrt(sq_sum_in_degree / send_load_dist.size() - mean_in_degree * mean_in_degree);
    double cv_in_degree = stddev_in_degree / mean_in_degree;

    // out-degree
    // min max
    uint32_t min_out_degree = *min_element(recv_load_dist.begin(), recv_load_dist.end());
    uint32_t max_out_degree = *max_element(recv_load_dist.begin(), recv_load_dist.end());
    // mean, stddev, cv
    double mean_out_degree = 1.0 * std::accumulate(recv_load_dist.begin(), recv_load_dist.end(), 0) / recv_load_dist.size();
    double sq_sum_out_degree = std::inner_product(recv_load_dist.begin(), recv_load_dist.end(), recv_load_dist.begin(), 0.0);
    double stddev_out_degree = std::sqrt(sq_sum_out_degree / recv_load_dist.size() - mean_out_degree * mean_out_degree);
    double cv_out_degree = stddev_out_degree / mean_out_degree;

    printf("================ Approach : %s =========================\n", approach.c_str());
    printf("send load: ");
    Utils::printVector(send_load_dist);
    printf("recv load: ");
    Utils::printVector(recv_load_dist);

    printf("send load: min: %u, max: %u, mean: %f, stddev: %f, cv: %f\n", min_in_degree, max_in_degree, mean_in_degree, stddev_in_degree, cv_in_degree);

    printf("recv load: min: %u, max: %u, mean: %f, stddev: %f, cv: %f\n", min_out_degree, max_out_degree, mean_out_degree, stddev_out_degree, cv_out_degree);

    printf("max_load: %u, bandwidth: %lu\n", max(max_in_degree, max_out_degree), total_bandwidth);

    return 0;
}