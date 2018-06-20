#ifndef QFC_BLOOMFILTER_HPP
#define QFC_BLOOMFILTER_HPP

#include <vector>
#include <exception>

#define NUM_QUOTIENT_BITS 32
#define NUM_REMAINDER_BITS 32

struct TooManyBucketsException : public exception {
  const char * what () const throw () {
    return "Quotient Filter initialized with too many buckets: cannot make stat array.";
  }
};

class QuotientFilter {
public:
  /**
   * Constructs a new Quotient Filter with the specified number of buckets,
   * using hash functions drawn from the indicated family of hash functions.
   */
  // QuotientFilter(size_t numBuckets, std::shared_ptr<HashFamily> family);
  QuotientFilter(size_t num, size_t bits, uint32_t seed)
                 : seed_(seed)
                 , q(NUM_QUOTIENT_BITS)
                 , r(NUM_REMAINDER_BITS) {
    numBucks = rndup(num*bits); // TODO: verify that there are no overflow issues here
    std::vector<uint64_t> temp(numBucks, 0); // TODO: move to initialization list?
    buckets = temp;
    if (3*numBucks <= numBucks)
      throw TooManyBucketsException();
    std::vector<bool> stat_temp(3*numBucks, false);
    stat_arr = stat_temp;
    num_elems = 0;
  }
  
  /**
   * Cleans up all memory allocated by this filter.
   */
  ~QuotientFilter() {};
 
  /**
   * Returns the quotient of f (q most significant bits).
   */
  uint64_t getQuotient(uint64_t f) const {
    return f >> r;
  }
  
  /**
   * Returns the remainder of f (r least significant bits).
   */
  uint64_t getRemainder(uint64_t f) const {
    return (f << q) >> q; 
  }
  
  /**
   * Function sets the three indicator bits for the given
   * index to the given value.
   */
  void set_3_bit(uint64_t ind, bool occ, bool cont, bool shift) {
    stat_arr[ind*3] = occ;
    stat_arr[ind*3+1] = cont;
    stat_arr[ind*3+2] = shift;
  }

  /**
   * Function takes in a bucket index and
   * decrements it, accounting for wrap around.
   */
  uint64_t decrement(uint64_t bucket) const {
    if (bucket == 0)
      return numBucks - 1;
    return bucket - 1;
  }
  
  /**
   * Function increments the given bucket according to the
   * wrap around rule.
   */
  uint64_t increment(uint64_t bucket) const {
    if (bucket >= numBucks - 1)
      return 0;
    return bucket + 1;
  }
  
  /**
   * Funciton returns whether or not a given
   * index is filled (true) or empty (false)
   */
  bool isFilled(uint64_t ind) const {
    return stat_arr[ind*3] || stat_arr[ind*3+1] || stat_arr[ind*3+2];
  }
  
  /**
   * Finds the run for the given bucket.
   */
  uint64_t find_run(uint64_t bucket) const {
    uint64_t temp_bucket = bucket;

    while (stat_arr[temp_bucket*3+2] == true) {
      temp_bucket = decrement(temp_bucket);
    }

    uint64_t start = temp_bucket;
    while(temp_bucket != bucket) {
      do {
        start = increment(start);
      } while (stat_arr[start*3+1] == true);

      do {
        temp_bucket = increment(temp_bucket);
      } while (!stat_arr[temp_bucket*3]);
    }

    return start;
  }

  /**
   * Inserts the specified element into filter. If the element already
   * exists, this operation is a no-op.
   */
  template<typename T>
  int insert(T data) {
    uint64_t f; MurmurHash3_x64_64((const void*) &data, sizeof(T), seed_, &f);

    // cout << "inserting hash: " << f << endl;
    uint64_t q_int = getQuotient(f);
    uint64_t r_int = getRemainder(f);
    uint64_t bucket = q_int % numBucks;
    
    // If full, cant insert.
    if (num_elems == numBucks)
        return -1;

    // If bucket empty, place.
    if (!isFilled(bucket)) {
      num_elems += 1;
      buckets[bucket] = r_int;
      set_3_bit(bucket, true, false, false);
      return 1;
    }
    
    // Checks if we are going to be first element in run 
    bool first_in_run = !stat_arr[bucket*3];
    // Set indicator to show filled
    stat_arr[bucket*3] = true;
    // Find run start
    uint64_t run_start = find_run(bucket);
    // Set variables used in running.
    bool inserted = false;
    bool last_cont = !first_in_run;
    bool run_over = false;
    
    while (isFilled(run_start) == true) {
      if (!inserted) {
        uint64_t temp = buckets[run_start];
        inserted = true;
        if (first_in_run) {
          stat_arr[run_start*3+1] = false;
          stat_arr[run_start*3+2] = (bucket != run_start);
          buckets[run_start] = r_int;
          r_int = temp;
          last_cont = false;
        } else if (run_over) { 
          stat_arr[run_start*3+1] = true;
          stat_arr[run_start*3+2] = true;
          buckets[run_start] = r_int;
          r_int = temp;
          last_cont = false;
        } else if (buckets[run_start] > r_int) {
          buckets[run_start] = r_int;
          r_int = temp;
          last_cont = true;
        } else if (buckets[run_start] == r_int) {
          return 1;
        } else {
          inserted = false;
        }
      } else {
        // save state
        uint64_t temp = buckets[run_start];
        bool temp_cont = stat_arr[run_start*3+1];
        stat_arr[run_start*3+1] = last_cont;
        stat_arr[run_start*3+2] = true;
        // shift back
        buckets[run_start] = r_int;
        r_int = temp;
        last_cont = temp_cont;
      }
      run_start = increment(run_start);
      if (stat_arr[run_start*3+1] == false)
        run_over = true;
    }
    
    // Insert
    buckets[run_start] = r_int;
    stat_arr[run_start*3+1] = last_cont;
    if (inserted) {
      stat_arr[run_start*3+2] = true;
    } else {
      stat_arr[run_start*3+2] = (bucket != run_start);
    }
      
    num_elems += 1;
    return 1;
  }

  /**
   * Returns whether the specified key is contained in the filter.
   */
  template<typename T>
  bool contains(T key) {
    uint64_t f; MurmurHash3_x64_64((const void*) &key, sizeof(T), seed_, &f);
    uint64_t q_int = getQuotient(f);
    uint64_t r_int = getRemainder(f);
    uint64_t bucket = q_int % numBucks;
    uint64_t run_start = bucket;
    uint64_t started = bucket;

    // If canonical slot is empty, return false
    if (stat_arr[bucket*3] == false)
      return false;
    
    // Loop while still looking at non-empty spots
    run_start = find_run(bucket);
    while (isFilled(run_start) == true) {
      // If we hit value, true!!
      if (buckets[run_start] == r_int)
        return true;
      // Because we are in sorted order, stop early if greater
      if (buckets[run_start] > r_int)
        return false;
      run_start = increment(run_start);
      // If our run is over, return false
      if (stat_arr[run_start*3+1] != true)
        return false;
      if (started == run_start)
        return false;
    }

    return false;
  }

private:
  uint32_t seed_;
  uint64_t numBucks;
  uint64_t num_elems;
  std::vector<uint64_t> buckets; // Array of buckets containing data
  int r, q; // number of remainder and quotient bits
  // TODO: verify that vector<bool> is space-efficient!
  std::vector<bool> stat_arr; // Use Bool array cause it only stores one bit 
  
  uint64_t rndup(uint64_t x) const {
    return ((x + 63) >> 6) << 6;
  }

  QuotientFilter(QuotientFilter const &) = delete;
  void operator=(QuotientFilter const &) = delete;
};

#endif // QFC_BLOOMFILTER_HPP