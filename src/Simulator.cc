#include "model/Stripe.hh"
#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"
#include "model/StripeBatch.hh"
#include "model/StripeGroup.hh"
#include "model/RecvBipartite.hh"
#include "util/Utils.hh"
#include "model/StripeMergeG.hh"
#include "model/BalancdConversion.hh"

int main(int argc, char *argv[])
{

    if (argc != 9)
    {
        printf("usage: ./Simulator k_i m_i k_f m_f N M placement_file approach[BT/SM]");
        return -1;
    }

    size_t k_i = atoi(argv[1]);
    size_t m_i = atoi(argv[2]);
    size_t k_f = atoi(argv[3]);
    size_t m_f = atoi(argv[4]);
    size_t N = atoi(argv[5]);
    size_t M = atoi(argv[6]);
    string placement_file = argv[7];
    string approach = argv[8];

    // random generator
    mt19937 random_generator = Utils::createRandomGenerator();

    // initialize code
    ConvertibleCode code(k_i, m_i, k_f, m_f);
    ClusterSettings settings;
    settings.N = N;
    settings.M = M;

    // check the number of stripes are valid
    if (Utils::isParamValid(code, settings) == false)
    {
        printf("invalid parameters\n");
        return -1;
    }

    // stripe generator
    StripeGenerator stripe_generator;

    // load stripes from placement file
    vector<Stripe> stripes;
    stripe_generator.loadStripes(code, settings, placement_file, stripes);

    // printf("stripes:\n");
    // for (size_t i = 0; i < stripes.size(); i++) {
    //     stripes[i].print();
    // }

    // solutions and load distribution
    vector<vector<size_t>> solutions;
    vector<size_t> send_load_dist, recv_load_dist;

    if (approach == "SM")
    {
        // stripe-merge-g

        // Step 1: enumerate all possible stripe groups; pick non-overlapped stripe groups in ascending order of transition costs (bandwidth)
        StripeBatch stripe_batch(0, code, settings);
        stripe_batch.constructByCost(stripes);
        stripe_batch.print();

        // Step 2: generate transition solutions from all stripe groups
        StripeMergeG stripe_merge_g;
        stripe_merge_g.getSolutionForStripeBatch(stripe_batch, solutions, random_generator);
    }
    else if (approach == "BT")
    {
        // balanced transition

        // Step 1: construct a stripe batch

        // possible construction techniques: sequentially construct; randomly construct; construct by cost
        StripeBatch stripe_batch(0, code, settings);

        // stripe_batch.constructInSequence(stripes);
        // stripe_batch.constructByRandomPick(stripes, random_generator);
        // stripe_batch.constructByCost(stripes);
        // stripe_batch.constructByCostAndSendLoad(stripes);
        stripe_batch.constructBySendLoadAndCost(stripes, random_generator);
        stripe_batch.print();

        BalancdConversion balanced_conversion;

        balanced_conversion.getSolutionForStripeBatchGlobal(stripe_batch, solutions, random_generator);

        // balanced_conversion.getSolutionForStripeBatchGreedy(stripe_batch, solutions, random_generator);

        // balanced_conversion.getSolutionForStripeBatchIter(stripe_batch, solutions, random_generator);
    }

    // get load distribution
    Utils::getLoadDist(code, settings, solutions, send_load_dist, recv_load_dist);

    // get bandwidth
    size_t bw = 0;
    for (auto item : send_load_dist)
    {
        bw += item;
    }

    size_t min_send_load = *min_element(send_load_dist.begin(), send_load_dist.end());
    size_t max_send_load = *max_element(send_load_dist.begin(), send_load_dist.end());
    size_t min_recv_load = *min_element(recv_load_dist.begin(), recv_load_dist.end());
    size_t max_recv_load = *max_element(recv_load_dist.begin(), recv_load_dist.end());

    printf("=====================================================\n");
    printf("%s send load distribution:, minimum_load: %ld, maximum_load: %ld\n", approach.c_str(), min_send_load, max_send_load);
    Utils::printUIntVector(send_load_dist);
    printf("%s recv load distribution:, minimum_load: %ld, maximum_load: %ld\n", approach.c_str(), min_recv_load, max_recv_load);
    Utils::printUIntVector(recv_load_dist);
    printf("%s bandwidth: %ld\n", approach.c_str(), bw);

    return 0;
}