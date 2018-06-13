#ifndef QuotientFilter_Included
#define QuotientFilter_Included

#include "Hashes.h"
#include <vector>
class QuotientFilter {
public:
  /**
   * Constructs a new Quotient Filter with the specified number of buckets,
   * using hash functions drawn from the indicated family of hash functions.
   */
  // QuotientFilter(size_t numBuckets, std::shared_ptr<HashFamily> family);
  QuotientFilter(size_t num, size_t bits, uint32_t seed);
  
  /**
   * Cleans up all memory allocated by this filter.
   */
  ~QuotientFilter();
 
  /**
   * Returns the quotient of f (q most significant bits).
   */
  uint64_t QuotientFilter::getQuotient(uint64_t f) const;
  
  /**
   * Returns the remainder of f (r least significant bits).
   */
  uint64_t QuotientFilter::getRemainder(uint64_t f) const;

  /**
   * Inserts the specified element into filter. If the element already
   * exists, this operation is a no-op.
   */
  template<typename T> int insert(T data);

  /**
   * Function sets the three indicator bits for the given
   * index to the given value.
   */
  void set_3_bit(uint64_t ind, bool occ, bool cont, bool shift);
  
  /**
   * Returns whether the specified key is contained in the filter.
   */
  template<typename T> bool contains(T key) const;
    
  /**
   * Function takes in a bucket index and
   * decrements it, accounting for wrap around.
   */
  uint64_t decrement(uint64_t bucket) const;
  
  
  /**
   * Funciton returns whether or not a given
   * index is filled (true) or empty (false)
   */
  bool isFilled(size_t ind) const;
  
  /**
   * Function increments the given bucket according to the
   * wrap around rule.
   */
  uint64_t increment(uint64_t bucket) const;
    
  /**
   * Finds the run for the given bucket.
   */
  uint64_t find_run(uint64_t bucket) const;

private:
  uint32_t seed_;
  uint64_t numBucks;
  uint64_t num_elems;
  std::vector<uint64_t> buckets; // Array of buckets containing data
  int r, q; // number of remainder and quotient bits
  // TODO: verify that vector<bool> is space-efficient!
  std::vector<bool> stat_arr; // Use Bool array cause it only stores one bit 
  
  QuotientFilter(QuotientFilter const &) = delete;
  void operator=(QuotientFilter const &) = delete;
};

#endif
