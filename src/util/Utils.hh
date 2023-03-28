#ifndef __UTILS_HH__
#define __UTILS_HH__

#include "../include/include.hh"
#include "../model/ConvertibleCode.hh"
#include "../model/ClusterSettings.hh"

class Utils
{
private:
    /* data */
public:
    Utils(/* args */);
    ~Utils();

    static bool isParamValid(const ConvertibleCode &code, const ClusterSettings &settings);

    static mt19937 createRandomGenerator();
    static size_t randomUInt(size_t l, size_t r, mt19937 &random_generator);

    static void printUintVector(vector<size_t> &vec);
    static void printUIntVector(vector<size_t> &vec);
    static void printUIntArray(size_t *arr, size_t len);
    static void printUIntMatrix(size_t **matrix, size_t r, size_t c);

    static size_t **initUIntMatrix(size_t r, size_t c);
    static void destroyUIntMatrix(size_t **matrix, size_t r, size_t c);

    static vector<size_t> argsortIntVector(vector<int> &vec);
    static vector<size_t> argsortUIntVector(vector<size_t> &vec);

    static void makeCombUtil(vector<vector<size_t>> &ans,
                             vector<size_t> &tmp, size_t n, size_t left, size_t k);
    static vector<vector<size_t>> getCombinations(size_t n, size_t k);

    static void makePermUtil(vector<vector<size_t>> &ans,
                             vector<size_t> tmp, size_t left, size_t right, size_t k);
    static vector<vector<size_t>> getPermutation(size_t n, size_t k);

    static void getLoadDist(ConvertibleCode &code, ClusterSettings &settings, vector<vector<size_t>> &solutions, vector<size_t> &send_load_dist, vector<size_t> &recv_load_dist);
};

#endif // __UTILS_HH__