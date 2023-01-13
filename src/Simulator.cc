#include "model/Stripe.hh"
#include "util/StripeGenerator.hh"

int main(int argc, char *argv[]) {

    if (argc != 5) {
        printf("usage: ./Simulator k m N M");
        return -1;
    }

    int k = atoi(argv[1]);
    int m = atoi(argv[2]);
    int N = atoi(argv[3]);
    int M = atoi(argv[4]);

    StripeGenerator stripe_generator;

    vector<Stripe> stripes = stripe_generator.GenerateStripes(k, m, N, M);

    printf("stripes:\n");

    for (size_t i = 0; i < stripes.size(); i++) {
        stripes[i].print();
    }
}