#include "QuotientFilter.h"
#include <iostream>


QuotientFilter::QuotientFilter(size_t numBuckets, 
                               std::shared_ptr<HashFamily> family):
                               buckets(numBuckets, 0){
  fp = family->get();
  //std::vector<int> temp(numBuckets, 0);
  //buckets = temp; 
  numBucks = numBuckets;
  r = 16;
  q = 16;
  std::vector<bool> stat_temp(3*numBuckets, false);
  stat_arr = stat_temp;
  num_elems = 0;
}

QuotientFilter::~QuotientFilter() {
  // TODO implement this
}

int QuotientFilter::getqr(int f, bool is_q) const {
  int mask = 0xffffffff;
  int ret = 0;
  if(is_q){
      mask = mask << r;
      ret = f & mask;
      ret = (ret >> r);
  }else {
      mask = mask >> (8*sizeof(int) - r);
      ret = f & mask;
  }
  return ret;
}

int QuotientFilter::insert(int data) {
  int f = fp(data);
  int q_int = getqr(f, true);
  uint16_t r_int = getqr(f, false);
  size_t bucket = q_int % numBucks;
  
  // If full, cant insert.
  if(num_elems == numBucks)
      return -1;

  // If bucket empty, place.
  if(!isFilled(bucket)) {
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
  size_t run_start = find_run(bucket);
  // Set variables used in running.
  bool inserted = false;
  bool last_cont = !first_in_run;
  bool run_over = false;
  
  while(isFilled(run_start) == true) {
    if(!inserted) {
        uint16_t temp = buckets[run_start];
        inserted = true;
        if(first_in_run) {
            stat_arr[(run_start*3) + 1] = false;
            stat_arr[(run_start*3) + 2] = (bucket != run_start);
            buckets[run_start] = r_int;
            r_int = temp;
            last_cont = false;
        }else if(run_over) { 
            stat_arr[(run_start*3) + 1] = true;
            stat_arr[(run_start*3) + 2] = true;
            buckets[run_start] = r_int;
            r_int = temp;
            last_cont = false;
        }else if((buckets[run_start] > r_int)) {
            buckets[run_start] = r_int;
            r_int = temp;
            last_cont = true;
        } else if(buckets[run_start] == r_int) {
            return 1;
        }else {
            inserted = false;
        }
    } else {
        // save state
        uint16_t temp = buckets[run_start];
        bool temp_cont = stat_arr[(run_start*3) + 1];
        stat_arr[(run_start*3) + 1] = last_cont;
        stat_arr[(run_start*3) + 2] = true;
        // shift back
        buckets[run_start] = r_int;
        r_int = temp;
        last_cont = temp_cont;
    }
    run_start = increment(numBucks, run_start);
    if(stat_arr[(run_start*3) + 1] == false)
        run_over = true;
  }
  
  // Insert
  buckets[run_start] = r_int;
  stat_arr[(run_start*3) + 1] = last_cont;
  if(inserted){
    stat_arr[(run_start*3) + 2] = true;
  } else {
    stat_arr[(run_start*3) + 2] = (bucket != run_start);
  }
    
  num_elems += 1;
  return 1;
}


bool QuotientFilter::contains(int data) const {
  int f = fp(data);
  int q_int = getqr(f, true);
  uint16_t r_int = getqr(f, false);
  size_t bucket = q_int % numBucks;
  size_t run_start = bucket;
  size_t started = bucket;

  // If connonical slot empty, return false
  if(stat_arr[bucket*3] == false)
      return false;
  
  // If full only choice is to linscan
  //if(num_elems == numBucks)
  //    return linscan(data, bucket);

  // Loop while still looking at non-empty spots
  run_start = find_run(bucket);
  while(isFilled(run_start) == true) {
    // If we hit value, true!!
    if(buckets[run_start] == r_int)
      return true;
    // Because we are in sorted order, stop early if greater
    if(buckets[run_start] > r_int)
      return false;
    run_start = increment(numBucks, run_start);
    // If our run is over, return false
    if(stat_arr[(run_start*3) + 1] != true)
        return false;
    if(started == run_start)
        return false;
  }
  return false;
}

bool QuotientFilter::linscan(int data, size_t bucket) const{
  int f = fp(data);
  uint16_t r_int = getqr(f, false);
  size_t spot = bucket;
  do{
    if(buckets[spot] == r_int)
        return true;
    spot = increment(numBucks, spot);
  }while(spot != bucket);
  
  // Code used for debugging!
  /*for(int j = 0; j < numBucks; j++) {
    std::string out  = "";
    out.resize(7);
    for(int k = 0; k < 3; k++){
      if(stat_arr[(j*3) + k] == false){
        out.push_back('0');
      }else{
        out.push_back('1');
      }
      out.push_back(' ');
    }
    std::cout<<"Index "<<j<<": ["<<out<<"]"<<std::endl;
  }*/
  return false;
}

void QuotientFilter::remove(int data) {
   // TODO implement this
}


// Below are helper functions for the main algorithms
bool QuotientFilter::isFilled(size_t ind) const{
  return stat_arr[ind*3] || stat_arr[(ind * 3) +1] || stat_arr[(ind*3) +2];
}


size_t QuotientFilter::decrement(size_t bucket) const{
  if(bucket == 0)
    return numBucks - 1;
  return bucket - 1;
}

void QuotientFilter::set_3_bit(size_t ind, bool occ,
                        bool cont, bool shift) {
  stat_arr[ind * 3] = occ;
  stat_arr[(ind * 3) + 1] = cont;
  stat_arr[(ind * 3) + 2] = shift;
}


size_t QuotientFilter::increment(size_t numBucks, size_t bucket) const {
  if(bucket >= numBucks - 1)
    return 0;
  return bucket + 1;
}

bool QuotientFilter::isClusterStart(size_t bucket) const {
  return (stat_arr[bucket*3] == true && stat_arr[(bucket*3) + 1] == false
          && stat_arr[(bucket*3) + 2] == false);
}

size_t QuotientFilter::find_run(size_t bucket) const{
  size_t temp_bucket = bucket;
  while(stat_arr[(temp_bucket*3) + 2] == true) {
    temp_bucket = decrement(temp_bucket);
  }
  size_t start = temp_bucket;
  while(temp_bucket != bucket) {
    do {
        start = increment(numBucks, start);
    } while(stat_arr[(start*3) + 1] == true);

    do {
        temp_bucket = increment(numBucks, temp_bucket);
    } while(!stat_arr[(temp_bucket*3)]);

  }
  return start;
}

