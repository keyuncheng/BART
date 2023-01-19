#include "Utils.hh"

Utils::Utils(/* args */)
{
}

Utils::~Utils()
{
}

bool Utils::isParamValid(const ConvertibleCode &code, const ClusterSettings &settings) {
    int num_stripes = settings.N;
    int num_nodes = settings.M;

    if (num_stripes % code.lambda_i != 0) {
        return false;
    }

    return true;
}

void Utils::print_int_vector(vector<int> &vec) {
    for (auto item : vec) {
        printf("%d ", item);
    }
    printf("\n");
}

void Utils::print_int_array(int *arr, int len) {
    for (int idx = 0; idx < len; idx++) {
        printf("%d ", arr[idx]);
    }
    printf("\n");
}