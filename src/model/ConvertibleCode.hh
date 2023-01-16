#ifndef __CONVERTIBLE_CODE_HH__
#define __CONVERTIBLE_CODE_HH__

typedef struct ConvertibleCode {
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
    
} ConvertibleCode;

#endif // __CONVERTIBLE_CODE_HH__