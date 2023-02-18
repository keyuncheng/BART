# BalancedConversion
Balanced Code Conversion

## Known issues

* When calculating costs, if m > 1, the cost for each parity block are
      calculated with ascending order of id, instead of ascending order of
      cost; not sure this can this be fixed?

* When number of stripes goes very large, the combinations cannot be
  represented by int
    * Type: int -> uint64_t

* Reducing the search space
    * filter out the groups with min loads worse than current best group