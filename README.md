# BalancedConversion
Balanced Code Conversion

## Known issues

* When number of stripes goes very large, the combinations cannot be
  represented by int
    * Type: int -> uint64_t

* Reducing the search space
    * filter out the groups with min loads worse than current best group