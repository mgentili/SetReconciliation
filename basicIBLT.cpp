#include <iostream>
#include <vector>
#include <stdint.h>
#include <assert.h>
#include <functional>
#include <cstdlib>
#include <deque>
#include <algorithm>

//--HASHFUNCTION STUFF

/**
Simple hash function family (a*x+b),
where a and b are randomly seeded odd integers
**/
template <typename key_type>
class simple_hash {
  public:
	int64_t a;
	int64_t b;

	simple_hash(): a(0), b(0) {}
	simple_hash( int seed ) {
		set_seed(seed);
	}

	void set_seed( int seed ) {
		srand(seed);
		while( a % 2 == 0 || a < (1 << 22) ) {
			a = rand();
		} 
		while( b % 2 == 0 || b < (1 << 22)) {
			b = rand();
		}
		//printf("seed is %d, a is %ld, b is %ld\n", seed, a, b);
	}

	key_type hash( int64_t k ) {
		return a*k + b;
	}
};

//--IBLT STUFF

//structure of a bucket within an IBLT
template <typename key_type>
class IBLT_bucket {
  public:
	key_type key_sum;
	key_type hash_sum;
	int count;

	IBLT_bucket(): key_sum(0), hash_sum(0), count(0) {}

	void add(key_type k, key_type h) {
		key_sum ^= k;
		hash_sum ^= h;
		count++;
	}

	void remove(key_type k, key_type h) {
		key_sum ^= k;
		hash_sum ^= h;
		count--;
	}

	void print_contents() {
		printf("key_sum: %ld, hash_sum: %ld, count: %d\n", key_sum, hash_sum, count);
	}
};

//TODO: make just one array, then decide on partitioning?
/**
Parameters:
	num_hashfns: number of hash functions (equivalent to k in the paper)
	hasher: type of hashfunction (should be able to hash keytype)
**/
template <typename key_type, typename hasher = simple_hash<key_type> >
class IBLT {
  public:
  	typedef IBLT_bucket<key_type> bucket_type;
  	typedef std::vector<bucket_type> IBLT_type;
  	std::vector<IBLT_type> subIBLTs;
  	hasher key_hasher;
  	std::vector<hasher> sub_hashers; 
	int num_buckets;
	int buckets_per_subIBLT;
	int num_hashfns;
	//seed num_hashfns different hashfunctions for each subIBLT
	IBLT(int bucket_count, int num_hashfns): 
							num_buckets(bucket_count), 
							num_hashfns(num_hashfns),
							buckets_per_subIBLT(num_buckets/num_hashfns),
							subIBLTs(num_hashfns),
							sub_hashers(num_hashfns)
							 {
		assert(num_buckets % num_hashfns == 0);
		key_hasher.set_seed(100);
		for(int i = 0; i < num_hashfns; ++i) {
			subIBLTs[i].resize(buckets_per_subIBLT);
			sub_hashers[i].set_seed((i + 1)*(1 << 20)); //separate seeds enough
		}
	}

	//insert a new key into our IBLT
	void insert_key(key_type key) {
		int bucket_index;
		key_type hashval = key_hasher.hash(key);
		for(int i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(key, i);
			subIBLTs[i][bucket_index].add( key, hashval);
		}
	}

	//peels the keys from an IBLT, returning true upon success, false upon failure
	//TODO: efficient way to do peeling?
	bool peel(std::vector<key_type>& peeled_keys ) {

		std::deque<bucket_type> peelable_keys;
		bucket_type curr_bucket;
		int unpeelable_count = 0;
		bool has_multiple_keys;
		//if we go through every entry and nothing is peelable, then we stop
		while( unpeelable_count != num_buckets ) {
			unpeelable_count = 0;
			has_multiple_keys = false;
			//first try to peel all the keys that can be peeled
			while( !peelable_keys.empty() ) {
				curr_bucket = peelable_keys.front();
				peeled_keys.push_back(curr_bucket.key_sum);
				peel_key( curr_bucket, peelable_keys );
				peelable_keys.pop_front();
			}
			//then try to find a key somewhere that can be peeled
			//TODO: should we break upon finding one?
			for(int i = 0; i < num_hashfns; ++i ) {
				for(int j = 0; j < buckets_per_subIBLT; ++j) {
					curr_bucket = subIBLTs[i][j];
					if( can_peel( curr_bucket ) ) {
						//printf("Can peel this bucket!!\n");
						peeled_keys.push_back(curr_bucket.key_sum);
						peel_key( curr_bucket, peelable_keys);
					} else {
						//curr_bucket.print_contents();
						if( curr_bucket.count != 0 )
							has_multiple_keys = true;
						unpeelable_count++;
					}
				}
			}
		}

		//either every bucket has one key or more, in which case we failed,
		//or every bucket has zero keys, in which case we succeeded;
		return !has_multiple_keys;
	}

//Below should be private at some point
	
	//need to think about design decision. Use bucket type when really only need key and hash.
	//only really need key, but for efficiency sake, want to keep hash along for the ride
	void peel_key(bucket_type& peelable_bucket, std::deque<bucket_type>& peelable_keys) {
		int bucket_index;
		for(int i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(peelable_bucket.key_sum, i);
			subIBLTs[i][bucket_index].remove(peelable_bucket.key_sum, peelable_bucket.hash_sum);
			if( can_peel(subIBLTs[i][bucket_index]) ) 
				peelable_keys.emplace_back(subIBLTs[i][bucket_index]);
		}
	}
	
	bool can_peel(bucket_type& curr_bucket) {
		if( abs(curr_bucket.count) != 1 ) {
			//printf("Current bucket count is: %d\n", curr_bucket.count);
			return false;
		}
		return( key_hasher.hash(curr_bucket.key_sum) == curr_bucket.hash_sum );
	}

	void get_bucket(key_type key, int index, bucket_type& curr_bucket) {
		curr_bucket = subIBLTs[index][get_bucket_index(key, index)];
	}

	//returns the bucket index of given key in given subIBLT
	int64_t get_bucket_index(key_type key, int subIBLT) {
		assert( subIBLT >= 0 && subIBLT < num_hashfns );
		return sub_hashers[subIBLT].hash(key) % buckets_per_subIBLT;
	}

	void print_contents() {
		for(int i = 0; i < num_hashfns; ++i) {
			for(int j = 0; j < buckets_per_subIBLT; ++j) {
				subIBLTs[i][j].print_contents();
			}
		}
	}

};

//--TESTING CODE--

/**  checkResults sorts the output of two vectors and ensures they are equal **/
void checkResults(std::vector<int64_t> expected, std::vector<int64_t> actual) {
	assert(expected.size() == actual.size() );
	std::sort( expected.begin(), expected.end() );
	std::sort( actual.begin(), actual.end() );
	for( int i = 0; i < expected.size(); ++i ) {
		assert( expected[i] == actual[i] );
	}
}

void run_trial(int seed, int num_hashfns, int num_buckets, int num_keys) {
	srand(seed);
	IBLT< int64_t > iblt(num_buckets, num_hashfns);
	std::vector<int64_t> keys_to_insert(num_keys);
	for(int i = 0; i < keys_to_insert.size(); ++i) {
		keys_to_insert[i] = rand();
		iblt.insert_key( keys_to_insert[i] );
	}
	//iblt.print_contents();
	std::vector<int64_t> peeled_keys;
	bool res = iblt.peel(peeled_keys);
	if( !res )
		printf("Failed to peel with %d keys, %d slots\n", num_keys, num_buckets);
	else {
		checkResults(keys_to_insert, peeled_keys);
		printf("Successfully peeled %d keys, %d slots\n", num_keys, num_buckets);
	}
}

/**
simulateIBLT tries inserting varying numbers of keys into the IBLT
and subsequently tries to peel. this helps in determining what the threshold
load is
**/
void simulateIBLT(int num_buckets, int num_trials ) {
	const int num_hfs = 4;
	for(int num_keys = 50; num_keys < num_buckets; num_keys*= 1.05) {
		for(int trial = 0; trial < num_trials; ++trial ) {
			int seed = trial* (1 << 25) + num_buckets;
			run_trial(seed, num_hfs, num_buckets, num_keys);
		}
	}
}

int main() {
	simulateIBLT(1 << 10, 1);
}