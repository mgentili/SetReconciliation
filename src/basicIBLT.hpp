#ifndef _BASIC_IBLT
#define _BASIC_IBLT

#include <vector>
#include <stdint.h>
#include <assert.h>
#include <functional>
#include <cstdlib>
#include <deque>
#include <algorithm>
#include <unordered_set>
#include "hash_util.hpp"
//--IBLT STUFF

//structure of a bucket within an IBLT
template <typename key_type, typename hash_type>
class basicIBLT_bucket {
  public:
	key_type key_sum;
	hash_type hash_sum;
	int count;

	basicIBLT_bucket(): key_sum(0), hash_sum(0), count(0) {}

	static size_t size_in_bits() {
		return( sizeof(key_type)*8 + sizeof(hash_type)*8 + sizeof(count) );
	}

	void add(key_type k, hash_type h, int n_times) {
		key_sum ^= k;
		hash_sum ^= h;
		count += n_times;
	}

	void add(key_type k, hash_type h) {
		add(k, h, 1);
	}

	void remove(key_type k, hash_type h) {
		add(k, h, -1);
	}

	void remove(const basicIBLT_bucket<key_type, hash_type>& b) {
		add(b.key_sum, b.hash_sum, (-1)*b.count);
	}

	void XOR(const basicIBLT_bucket<key_type, hash_type>& b) {
		remove(b);
	}

	void print_contents() const {
		printf("key_sum: %ld, hash_sum: %ld, count: %d\n", key_sum, hash_sum, count);
	}

	bool is_empty() const {
		return (key_sum == 0) && (hash_sum == 0) && (count == 0);
	}

    bool operator==( const basicIBLT_bucket& other ) const
    {
    	return( key_sum == other.key_sum && hash_sum == other.hash_sum );
    }

    bool operator<( const basicIBLT_bucket& other ) const 
	{
		return( key_sum < other.key_sum || hash_sum < other.hash_sum );
	}
};

/**
Parameters:
	num_hashfns: number of hash functions (equivalent to k in the paper)
	hasher: type of hashfunction (should be able to hash keytype)
**/
template <typename key_type, 
		  typename hash_type, 
		  typename hasher = MurmurHashing<8*sizeof(key_type), hash_type> >
class basicIBLT {
  public:
  	typedef basicIBLT_bucket<key_type, hash_type> bucket_type;
  	typedef std::vector<bucket_type> IBLT_type;
  	size_t num_buckets;
	size_t num_hashfns;
	size_t buckets_per_subIBLT;
  	std::vector<IBLT_type> subIBLTs;
  	hasher key_hasher;
  	std::vector<hasher> sub_hashers; 

	//seed num_hashfns different hashfunctions for each subIBLT
	basicIBLT(size_t bucket_count, size_t num_hashfns): 
							num_buckets(bucket_count), 
							num_hashfns(num_hashfns),
							subIBLTs(num_hashfns),
							sub_hashers(num_hashfns) {
		while( num_buckets % num_hashfns != 0) {
			++num_buckets;
		}
		assert(num_buckets % num_hashfns == 0);
		buckets_per_subIBLT = num_buckets/num_hashfns;
		key_hasher.set_seed(0);
		for(size_t i = 0; i < num_hashfns; ++i) {
			subIBLTs[i].resize(buckets_per_subIBLT);
			sub_hashers[i].set_seed(i+1); //separate seeds enough
		}
	}

	size_t size_in_bits() const {
		return( num_buckets * bucket_type::size_in_bits());
	}

	void XOR(const basicIBLT<key_type, hash_type>& counterparty) {
		assert( counterparty.buckets_per_subIBLT == buckets_per_subIBLT 
			&&  counterparty.num_hashfns == num_hashfns);
		for(size_t i = 0; i < num_hashfns; ++i) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				subIBLTs[i][j].XOR( counterparty.subIBLTs[i][j] );
			}
		}
	}

	//insert a new key into our IBLT
	void insert_key(key_type key) {
		size_t bucket_index;
		hash_type hashval = key_hasher.hash(key);
		for(size_t i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(key, i);
			subIBLTs[i][bucket_index].add( key, hashval);
		}
	}

	void insert_keys(const std::unordered_set<key_type>& keys) {
		for(auto it = keys.begin(); it!= keys.end(); ++it) {
			insert_key(*it);
		}
	}

	void remove_key(key_type key) {
		size_t bucket_index;
		hash_type hashval = key_hasher.hash(key);
		for(size_t i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(key, i);
			subIBLTs[i][bucket_index].remove( key, hashval);
		}
	}	

	bool peel(std::unordered_set<key_type>& my_peeled_keys, std::unordered_set<key_type>& cp_peeled_keys ) {
		std::deque<bucket_type> peelable_keys;
		bucket_type curr_bucket;

		//if we go through every entry and nothing is peelable, then we stop
		while( true ) {
			//first try to peel all the keys that can be peeled
			while( !peelable_keys.empty() ) {
				curr_bucket = peelable_keys.front();
				peelable_keys.pop_front();
				key_type peeled_key = curr_bucket.key_sum;
				if( curr_bucket.count == -1) { //counterparty's key
					if( cp_peeled_keys.find(peeled_key) == cp_peeled_keys.end() ) {
						cp_peeled_keys.insert(peeled_key);
						peel_key( curr_bucket, peelable_keys );
					}
				} else {
					assert(curr_bucket.count == 1);
					if( my_peeled_keys.find(peeled_key) == my_peeled_keys.end() ) {
						my_peeled_keys.insert(peeled_key);
						peel_key( curr_bucket, peelable_keys );
					}
				}
			}

			//either every bucket has one key or more, in which case we failed,
			//or every bucket has zero keys, in which case we succeeded;
			if( !find_peelable_key(peelable_keys) ) {
				return( is_empty() );
			}
		}

		//can't reach this point
		assert(1 == 2);
		return false;
	}

	//peels the keys from an IBLT, returning true upon success, false upon failure
	//TODO: efficient way to do peeling?
	bool peel(std::unordered_set<key_type>& peeled_keys ) {
		std::unordered_set<key_type> my_peeled_keys, cp_peeled_keys;
		bool res = peel(my_peeled_keys, cp_peeled_keys);
		if( !res ) {
			std::cout << "Failed to peel" << std::endl;
			return false;
		}

		for(auto it = my_peeled_keys.begin(); it != my_peeled_keys.end(); ++it) {
			peeled_keys.insert(*it);
		}

		for(auto it = cp_peeled_keys.begin(); it != cp_peeled_keys.end(); ++it) {
			peeled_keys.insert(*it);
		}
		return true;
	}

	bool find_peelable_key(std::deque<bucket_type>& peelable_keys) {
		bool peeled_key = false;
		for(size_t i = 0; i < num_hashfns; ++i ) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				if( can_peel( subIBLTs[i][j] ) ) {
					peelable_keys.emplace_back(subIBLTs[i][j]);
					peeled_key = true;
				}
			}
		}
		return peeled_key;
	}

//Below should be private at some point
	//need to think about design decision. Use bucket type when really only need key and hash.
	//only really need key, but for efficiency sake, want to keep hash along for the ride
	void peel_key(bucket_type& peelable_bucket, std::deque<bucket_type>& peelable_keys) {
		size_t bucket_index;
		for(size_t i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(peelable_bucket.key_sum, i);
			subIBLTs[i][bucket_index].remove(peelable_bucket);
		}
	}
	
	bool can_peel(bucket_type& curr_bucket) {
		return abs(curr_bucket.count) == 1 && (key_hasher.hash(curr_bucket.key_sum) == curr_bucket.hash_sum);
	}

	//returns the bucket index of given key in given subIBLT
	size_t get_bucket_index(key_type key, size_t subIBLT) {
		return sub_hashers[subIBLT].hash(key) % buckets_per_subIBLT;
	}

	bool is_empty() const {
		for(size_t i = 0; i < num_hashfns; ++i) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				if( !subIBLTs[i][j].is_empty()) {
					return false;
				}
			}
		}
		return true;
	}

	void print_contents() const {
		for(size_t i = 0; i < num_hashfns; ++i) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				subIBLTs[i][j].print_contents();
			}
		}
	}
};


#endif