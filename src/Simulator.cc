#include "model/Stripe.hh"
#include "model/ConvertibleCode.hh"
#include "model/ClusterSettings.hh"
#include "util/StripeGenerator.hh"

int main(int argc, char *argv[]) {

    if (argc != 7) {
        printf("usage: ./Simulator k_i m_i k_f i_f N M");
        return -1;
    }

    int k_i = atoi(argv[1]);
    int m_i = atoi(argv[2]);
    int k_f = atoi(argv[3]);
    int m_f = atoi(argv[4]);
    int N = atoi(argv[5]);
    int M = atoi(argv[6]);

    StripeGenerator stripe_generator;

    ConvertibleCode code;
    ClusterSettings settings;

    // code parameters
    code.k_i = k_i;
    code.m_i = m_i;
    code.n_i = k_i + m_i;

    code.k_f = k_f;
    code.m_f = m_f;
    code.n_f = k_f + m_f;

    code.alpha = k_f / k_i;
    code.beta = k_f - code.alpha * k_i;

    settings.N = N;
    settings.M = M;

    vector<Stripe> stripes = stripe_generator.GenerateStripes(code, settings);

    printf("stripes:\n");

    for (auto i = 0; i < stripes.size(); i++) {
        stripes[i].print();
    }
}