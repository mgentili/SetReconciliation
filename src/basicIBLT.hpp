#ifndef BASIC_IBLT
#define BASIC_IBLT

#include <vector>
#include <stdint.h>
#include <assert.h>
#include <functional>
#include <cstdlib>
#include <deque>
#include <algorithm>
#include <unordered_set>
#include "tabulation_hashing.hpp"
//--IBLT STUFF


//structure of a bucket within an IBLT
template <typename key_type, typename hash_type = uint64_t>
class basicIBLT_bucket {
  public:
	key_type key_sum;
	hash_type hash_sum;
	int count;

	basicIBLT_bucket(): key_sum(0), hash_sum(0), count(0) {}

	void add(key_type k, hash_type h) {
		key_sum ^= k;
		hash_sum ^= h;
		count++;
	}

	void remove(key_type k, hash_type h) {
		key_sum ^= k;
		hash_sum ^= h;
		count--;
	}

	void remove(basicIBLT_bucket<key_type> b) {
		key_sum ^= b.key_sum;
		hash_sum ^= b.hash_sum;
		count += (-1)*b.count;
	}

	void XOR(basicIBLT_bucket<key_type> counterparty_bucket) {
		key_sum ^= counterparty_bucket.key_sum;
		hash_sum ^= counterparty_bucket.hash_sum;
		count -= counterparty_bucket.count;
	}

	void print_contents() const {
		printf("key_sum: %ld, hash_sum: %ld, count: %d\n", key_sum, hash_sum, count);
	}

    bool operator==( const basicIBLT_bucket& other ) const
    {
    	return( key_sum == other.key_sum && hash_sum == other.hash_sum );
    }

};

//TODO: make just one array, then decide on partitioning?
/**
Parameters:
	num_hashfns: number of hash functions (equivalent to k in the paper)
	hasher: type of hashfunction (should be able to hash keytype)
**/
template <typename key_type, typename hash_type = uint64_t, typename hasher = TabulationHashing<8*sizeof(key_type), hash_type> >
//template <typename key_type, typename hash_type = uint64_t, typename hasher = simple_hash<key_type> >
class basicIBLT {
  public:
  	typedef basicIBLT_bucket<key_type, hash_type> bucket_type;
  	typedef std::vector<bucket_type> IBLT_type;
  	int num_buckets;
	int num_hashfns;
	int buckets_per_subIBLT;
  	std::vector<IBLT_type> subIBLTs;
  	hasher key_hasher;
  	std::vector<hasher> sub_hashers; 

	//seed num_hashfns different hashfunctions for each subIBLT
	basicIBLT(int bucket_count, int num_hashfns): 
							num_buckets(bucket_count), 
							num_hashfns(num_hashfns),
							buckets_per_subIBLT(num_buckets/num_hashfns),
							subIBLTs(num_hashfns),
							sub_hashers(num_hashfns) {
		assert(num_buckets % num_hashfns == 0);
		key_hasher.set_seed(0);
		for(int i = 0; i < num_hashfns; ++i) {
			subIBLTs[i].resize(buckets_per_subIBLT);
			sub_hashers[i].set_seed(i+1); //separate seeds enough
		}
	}

	void XOR(basicIBLT<key_type>& counterparty) {
		assert( counterparty.buckets_per_subIBLT == buckets_per_subIBLT 
			&&  counterparty.num_hashfns == num_hashfns);
		for(int i = 0; i < num_hashfns; ++i) {
			for(int j = 0; j < buckets_per_subIBLT; ++j) {
				subIBLTs[i][j].XOR( counterparty.subIBLTs[i][j] );
			}
		}

	}
	//insert a new key into our IBLT
	void insert_key(key_type key) {
		int bucket_index;
		hash_type hashval = key_hasher.hash(&key);
		for(int i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(key, i);
			
			subIBLTs[i][bucket_index].add( key, hashval);
		}
	}

	void remove_key(key_type key) {
		int bucket_index;
		hash_type hashval = key_hasher.hash(&key);
		for(int i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(key, i);
			
			subIBLTs[i][bucket_index].remove( key, hashval);
		}
	}	

	//peels the keys from an IBLT, returning true upon success, false upon failure
	//TODO: efficient way to do peeling?
	bool peel(std::unordered_set<key_type>& peeled_keys ) {
		std::deque<bucket_type> peelable_keys;
		bucket_type curr_bucket;
		bool has_multiple_keys;
		//if we go through every entry and nothing is peelable, then we stop
		while( true ) {
			//first try to peel all the keys that can be peeled
			while( !peelable_keys.empty() ) {
				curr_bucket = peelable_keys.front();
				peelable_keys.pop_front();
				if( peeled_keys.insert(curr_bucket.key_sum).second )
					peel_key( curr_bucket, peelable_keys );
				/*else
					printf("Already peeled this key!\n");*/
			}

			//either every bucket has one key or more, in which case we failed,
			//or every bucket has zero keys, in which case we succeeded;
			if( !find_peelable_key(peelable_keys, has_multiple_keys) ) {
				//printf("Peelable keys size is %d\n", peelable_keys.size());
				return !has_multiple_keys;
			}
		}

		//can't reach this point
		assert(1 == 2);
		return false;
	}

	// returns whether was able to find peelable key
	// stores whether during search 
	bool find_peelable_key(std::deque<bucket_type>& peelable_keys, bool& has_multiple_keys) {
		has_multiple_keys = false;
		for(int i = 0; i < num_hashfns; ++i ) {
			for(int j = 0; j < buckets_per_subIBLT; ++j) {
				if( can_peel( subIBLTs[i][j] ) ) {
					peelable_keys.emplace_back(subIBLTs[i][j]);
					return true;
				}
				if( subIBLTs[i][j].count != 0 )
					has_multiple_keys = true;
			}
		}
		return false;
	}
//Below should be private at some point
	
	//need to think about design decision. Use bucket type when really only need key and hash.
	//only really need key, but for efficiency sake, want to keep hash along for the ride
	void peel_key(bucket_type& peelable_bucket, std::deque<bucket_type>& peelable_keys) {
		int bucket_index;
		for(int i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(peelable_bucket.key_sum, i);
			subIBLTs[i][bucket_index].remove(peelable_bucket);

			//optimization: if find a new peelable bucket, add it to queue
			//need to be careful that don't add same key twice
			if( can_peel(subIBLTs[i][bucket_index]) ) {
				//printf("Trying to peel key from other buckets\n");
				peelable_keys.emplace_back(subIBLTs[i][bucket_index]);
			}
		}
	}
	
	bool can_peel(bucket_type& curr_bucket) {
		if( abs(curr_bucket.count) != 1 ) {
			//printf("Current bucket count is: %d\n", curr_bucket.count);
			return false;
		}
		return( key_hasher.hash(&curr_bucket.key_sum) == curr_bucket.hash_sum );
	}

	void get_bucket(key_type key, int index, bucket_type& curr_bucket) {
		curr_bucket = subIBLTs[index][get_bucket_index(key, index)];
	}

	//returns the bucket index of given key in given subIBLT
	uint64_t get_bucket_index(key_type key, int subIBLT) {
		assert( subIBLT >= 0 && subIBLT < num_hashfns );
		return sub_hashers[subIBLT].hash(&key) % buckets_per_subIBLT;
	}

	void print_contents() {
		for(int i = 0; i < num_hashfns; ++i) {
			for(int j = 0; j < buckets_per_subIBLT; ++j) {
				subIBLTs[i][j].print_contents();
			}
		}
	}
};


#endif