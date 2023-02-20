# BalancedConversion
Balanced Code Conversion

## Known issues

* When number of stripes goes very large, the combinations cannot be
  represented by int
    * Type: int -> uint64_t

* The approach enumeration space is too large
    * e.g. 100 SGs, the approach enumeration = 2^100 = 1.2676506e+30, can't be
      represented by uint64_t

* Reducing the search space
    * filter out the groups with min loads worse than current best group