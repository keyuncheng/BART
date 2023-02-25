# BalancedConversion
Balanced Code Conversion

## Known issues

* The current greedy algorithm for finding the load-balanced solution is not
  good
    * Need to implement a alternating path based algorithm

* Reducing the search space: The approach enumeration space is too large
    * e.g. 100 SGs, the approach enumeration = 2^100 = 1.2676506e+30
    * Implemented two heuristics to reduce the search space (Feb 25)
        * Greedy
        * Iterative replace

* Handle re-encoding only coding schemes: lambda_f > 1
    * Need to make sure that each of lambda_f stripes have k_f data blocks
    * Need to find lambda_f locations for computation
    * Need to relocate for lambda_f * m_f parity blocks