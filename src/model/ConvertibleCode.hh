#ifndef __CONVERTIBLE_CODE_HH__
#define __CONVERTIBLE_CODE_HH__

#include "../include/include.hh"

class ConvertibleCode
{
private:
    /* data */
public:

        // input ec parameters (k, m)
    int k_i;
    int m_i;
    int n_i;

    // output ec paramters (k', m')
    int k_f;
    int m_f;
    int n_f;

    // k' = alpha k + beta
    int alpha;
    int beta;
    
    // theta = lcm(k, k')
    int theta;
    int lambda_i;
    int lambda_f;

    ConvertibleCode(/* args */);
    ConvertibleCode(int k_in, int m_in, int k_out, int m_out);
    ~ConvertibleCode();

private:
    int lcm(int a, int b);
};

#endif // __CONVERTIBLE_CODE_HH__