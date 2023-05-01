#include "model/Stripe.hh"
#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "model/StripeBatch.hh"
// #include "model/StripeGroup.hh"
// #include "model/RecvBipartite.hh"
// #include "util/Utils.hh"
// #include "model/StripeMergeG.hh"
// #include "model/BalancedConversion.hh"

int main(int argc, char *argv[])
{

    if (argc != 9)
    {
        printf("usage: ./Simulator k_i m_i k_f m_f M N placement_file approach[BT/SM]");
        return -1;
    }

    uint8_t k_i = atoi(argv[1]);
    uint8_t m_i = atoi(argv[2]);
    uint8_t k_f = atoi(argv[3]);
    uint8_t m_f = atoi(argv[4]);
    uint16_t num_nodes = atoi(argv[5]);
    uint32_t num_stripes = atoi(argv[6]);
    string placement_file = argv[7];
    string approach = argv[8];

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

    // stripe generator
    StripeGenerator stripe_generator;

    // load stripes from placement file
    vector<Stripe> stripes;
    stripe_generator.loadStripes(code, settings, placement_file, stripes);

    printf("finished loading stripes\n");

    // printf("stripes:\n");
    // for (auto &stripe : stripes)
    // {
    //     stripe.print();
    // }

    // solutions and load distribution
    vector<vector<size_t>> solutions;
    vector<size_t> send_load_dist, recv_load_dist;

    if (approach == "SM")
    {

        // Step 1: enumerate all possible stripe groups; pick non-overlapped stripe groups in ascending order of transition costs (bandwidth)
        StripeBatch stripe_batch(0, code, settings, random_generator, stripes);
        // stripe_batch.constructSGInSequence();
        // stripe_batch.constructSGByRandomPick();
        stripe_batch.constructSGByCost();
        stripe_batch.print();

        // // Step 2: generate transition solutions from all stripe groups
        // StripeMergeG stripe_merge_g;
        // stripe_merge_g.getSolutionForStripeBatch(stripe_batch, solutions, random_generator);
    }
    // else if (approach == "BT")
    // {
    //     // balanced transition

    //     // Step 1: construct a stripe batch

    //     // possible construction techniques: sequentially construct; randomly construct; construct by cost
    //     StripeBatch stripe_batch(0, code, settings);

    //     // stripe_batch.constructInSequence(stripes);
    //     // stripe_batch.constructByRandomPick(stripes, random_generator);
    //     stripe_batch.constructByCost(stripes);
    //     // stripe_batch.constructByCostAndSendLoad(stripes);
    //     // stripe_batch.constructBySendLoadAndCost(stripes, random_generator);
    //     // stripe_batch.constructBySendLoadAndCostv2(stripes, random_generator);
    //     stripe_batch.print();

    //     BalancdConversion balanced_conversion;

    //     // balanced_conversion.getSolutionForStripeBatchGlobal(stripe_batch, solutions, random_generator);

    //     // balanced_conversion.getSolutionForStripeBatchGreedy(stripe_batch, solutions, random_generator);

    //     // balanced_conversion.getSolutionForStripeBatchIter(stripe_batch, solutions, random_generator);

    //     balanced_conversion.getSolutionForStripeBatchAssigned(stripe_batch, solutions, random_generator);

    //     size_t num_sg = stripe_batch.getStripeGroups().size();
    //     size_t num_sg_re = 0;
    //     size_t num_sg_pm = 0;
    //     double re_percent = 0;
    //     double pm_percent = 0;
    //     for (auto &item : stripe_batch.getSGApproaches())
    //     {
    //         if (item == EncodeMethod::RE_ENCODE)
    //         {
    //             num_sg_re++;
    //         }
    //         else
    //         {
    //             num_sg_pm++;
    //         }
    //     }
    //     re_percent = 1.0 * num_sg_re / num_sg;
    //     pm_percent = 1.0 * num_sg_pm / num_sg;
    //     printf("approach distribution: re-encoding: %ld / %ld (%f), parity merging: %ld / %ld (%f)\n", num_sg_re, num_sg, re_percent, num_sg_pm, num_sg, pm_percent);
    // }

    // // get load distribution
    // Utils::getLoadDist(code, settings, solutions, send_load_dist, recv_load_dist);

    // // get bandwidth
    // size_t total_bandwidth = 0;
    // for (auto item : send_load_dist)
    // {
    //     total_bandwidth += item;
    // }

    // // in-degree
    // // min, max
    // double min_in_degree = *min_element(send_load_dist.begin(), send_load_dist.end());
    // double max_in_degree = *max_element(send_load_dist.begin(), send_load_dist.end());
    // // mean, stddev, cv
    // double mean_in_degree = std::accumulate(send_load_dist.begin(), send_load_dist.end(), 0) / send_load_dist.size();
    // double sq_sum_in_degree = std::inner_product(send_load_dist.begin(), send_load_dist.end(), send_load_dist.begin(), 0.0);
    // double stddev_in_degree = std::sqrt(sq_sum_in_degree / send_load_dist.size() - mean_in_degree * mean_in_degree);
    // double cv_in_degree = stddev_in_degree / mean_in_degree;

    // // out-degree
    // // min max
    // double min_out_degree = *min_element(recv_load_dist.begin(), recv_load_dist.end());
    // double max_out_degree = *max_element(recv_load_dist.begin(), recv_load_dist.end());
    // // mean, stddev, cv
    // double mean_out_degree = 1.0 * std::accumulate(recv_load_dist.begin(), recv_load_dist.end(), 0) / recv_load_dist.size();
    // double sq_sum_out_degree = std::inner_product(recv_load_dist.begin(), recv_load_dist.end(), recv_load_dist.begin(), 0.0);
    // double stddev_out_degree = std::sqrt(sq_sum_out_degree / recv_load_dist.size() - mean_out_degree * mean_out_degree);
    // double cv_out_degree = stddev_out_degree / mean_out_degree;

    // printf("================ Approach : %s =========================\n", approach.c_str());
    // printf("send load: ");
    // Utils::printUIntVector(send_load_dist);
    // printf("recv load: ");
    // Utils::printUIntVector(recv_load_dist);

    // printf("send load: bandwidth: %ld, min: %f, max: %f, mean: %f, stddev: %f, cv: %f\n", total_bandwidth, min_in_degree, max_in_degree, mean_in_degree, stddev_in_degree, cv_in_degree);

    // printf("recv load: bandwidth: %ld, min: %f, max: %f, mean: %f, stddev: %f, cv: %f\n", total_bandwidth, min_out_degree, max_out_degree, mean_out_degree, stddev_out_degree, cv_out_degree);

    return 0;
}