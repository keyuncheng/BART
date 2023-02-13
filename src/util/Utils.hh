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
    static int randomInt(int l, int r, mt19937 &random_generator);

    static void printIntVector(vector<int> &vec);
    static void printIntArray(int *arr, int len);
    static void printIntMatrix(int **matrix, int r, int c);

    static int **initIntMatrix(int r, int c);
    static void destroyIntMatrix(int **matrix, int r, int c);

    static vector<int> argsortIntVector(vector<int> &vec);

    static void makeCombUtil(vector<vector<int> >& ans,
    vector<int>& tmp, int n, int left, int k);
    static vector<vector<int> > getCombinations(int n, int k);
};




#endif // __UTILS_HH__