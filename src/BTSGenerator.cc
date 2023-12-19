#include "model/Stripe.hh"
#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "model/StripeBatch.hh"
#include "model/RandomSolution.hh"
#include "model/BWOptSolution.hh"
#include "model/BART.hh"
#include "util/Stats.hh"

int main(int argc, char *argv[])
{
    if (argc != 11 && argc != 12)
    {
        printf("usage: ./BTSGenerator k_i m_i k_f m_f num_nodes num_stripes approach[RDRE/RDPM/BWRE/BWPM/BTRE/BTPM/BT/BTWeighted] pre_placement_filename post_placement_filename sg_meta_filename [bw_filename]\n");
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
    string bw_filename;

    // heterogeneous network
    bool is_heterogeneous = false;
    if (argc == 12)
    {
        bw_filename = argv[11];
        is_heterogeneous = true;
    }

    // random generator
    mt19937 random_generator = Utils::createRandomGenerator();

    // initialize code
    ConvertibleCode code(k_i, m_i, k_f, m_f);
    ClusterSettings settings(num_nodes, num_stripes);

    // load heterogeneous network settings
    if (is_heterogeneous == true)
    {
        if (settings.loadBWProfile(bw_filename) == false)
        {
            return -1;
        }
    }

    code.print();
    settings.print();

    // check the number of stripes are valid
    if (code.isValidForPM() == false || settings.isParamValid(code) == false)
    {
        printf("invalid parameters\n");
        return -1;
    }

    if (approach == "BTWeighted")
    {
        if (argc != 12)
        {
            printf("For BTWeighted, please explicitly specify the network settings <bw_filename>\n");
            return -1;
        }
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
    else if (approach == "BTRE" || approach == "BTPM" || approach == "BT" || approach == "BTWeighted")
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

    printf("================ Approach : %s =========================\n", approach.c_str());

    if (is_heterogeneous == true)
    {
        Stats::printWeightedLoadStats(transfer_load_dist, settings);
    }
    else
    {
        Stats::printLoadStats(transfer_load_dist);
    }

    return 0;
}