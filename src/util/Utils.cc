#include "Utils.hh"

Utils::Utils(/* args */)
{
}

Utils::~Utils()
{
}

bool Utils::isParamValid(const ConvertibleCode &code, const ClusterSettings &settings) {
    int num_stripes = settings.N;

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
    if (len <= 0) {
        printf("invalid input array dimensions\n");
        return;
    }
    for (int idx = 0; idx < len; idx++) {
        printf("%d ", arr[idx]);
    }
    printf("\n");
}

void Utils::print_int_matrix(int **matrix, int r, int c) {
    if (r <= 0 || c <= 0) {
        printf("invalid input matrix dimensions\n");
        return;
    }

    for (int rid = 0; rid < r; rid++) {
        for (int cid = 0; cid < c; cid++) {
            printf("%d ", matrix[rid][cid]);
        }
        printf("\n");
    }
    printf("\n");
}

int **Utils::init_int_matrix(int r, int c) {
    if (r <= 0 || c <= 0) {
        printf("invalid input matrix dimensions\n");
        return NULL;
    }

    int **matrix = (int **) malloc(r * sizeof(int *));
    for (int rid = 0; rid < r; rid++) {
        matrix[rid] = (int *) calloc(c, sizeof(int));
    }

    return matrix;
}

void Utils::destroy_int_matrix(int **matrix, int r, int c) {
    if (matrix == NULL) {
        return;
    }

    for (int rid = 0; rid < r; rid++) {
        free(matrix[rid]);
    }

    free(matrix);
}