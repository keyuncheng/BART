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

    static void print_int_vector(vector<int> &vec);
    static void print_int_array(int *arr, int len);
    static void print_int_matrix(int **matrix, int r, int c);

    static int **init_int_matrix(int r, int c);
    static void destroy_int_matrix(int **matrix, int r, int c);
};




#endif // __UTILS_HH__