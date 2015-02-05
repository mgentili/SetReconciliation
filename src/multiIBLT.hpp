#ifndef _MULTI_IBLT
#define _MULTI_IBLT

#include <vector>
#include <stdint.h>
#include <assert.h>
#include <functional>
#include <cstdlib>
#include <deque>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <bitset>
#include <unordered_map>
#include "tabulation_hashing.hpp"
#include "hash_util.hpp"
#include "basicField.hpp"
#include "file_sync.pb.h"

//structure of a bucket within an IBLT
template <size_t n_parties = 2, typename key_type = uint32_t, size_t key_bits= 8*sizeof(key_type), typename hash_type = uint32_t>
class multiIBLT_bucket {
  public:
  	typedef multiIBLT_bucket<n_parties, key_type, key_bits, hash_type> this_bucket_type;
	Field<n_parties, key_type, key_bits> key_sum;
	Field<n_parties, hash_type, 8*sizeof(hash_type)> hash_sum;
	SimpleField<n_parties> count;
	multiIBLT_bucket(): key_sum(), hash_sum(), count() {}

	size_t size_in_bits() {
		//TODO: THEORETICAL
		return( key_bits + 8*sizeof(hash_type) + 8*sizeof(int));
	}

	void serialize(file_sync::IBLT_bucket& bucket) {
		bucket.set_key_sum(key_sum.arg);
		bucket.set_hash_sum(hash_sum.arg);
		bucket.set_count(count.arg);
	}

	void deserialize(const file_sync::IBLT_bucket& bucket) {
		key_sum.arg = bucket.key_sum();
		hash_sum.arg = bucket.hash_sum();
		count.arg = bucket.count();
	}
	
	void multiply(int i) {
		key_sum.multiply(i);
		hash_sum.multiply(i);
		count.multiply(i);
	}
	void add(const key_type& k, const hash_type& h) {
		key_sum.add(k);
		hash_sum.add(h);
		count.add(1);
	}

	void add(const this_bucket_type& counterparty_bucket) {
		key_sum.add(counterparty_bucket.key_sum);
		hash_sum.add(counterparty_bucket.hash_sum);
		count.add(counterparty_bucket.count);
	}

	void add(const this_bucket_type& counterparty_bucket, size_t index) {
		add(counterparty_bucket);
	}

	void remove(const key_type& k, const hash_type& h) {
		key_sum.remove(k);
		hash_sum.remove(h);
		count.remove(1);
	}

	void remove(const this_bucket_type& counterparty_bucket) {
		key_sum.remove(counterparty_bucket.key_sum);
		hash_sum.remove(counterparty_bucket.hash_sum);
		count.remove(counterparty_bucket.count);
	}

	void remove(const this_bucket_type& counterparty_bucket, size_t index) {
		remove(counterparty_bucket);
	}

	void print_contents() const {
		printf("Key_sum:");
		key_sum.print_contents();
		printf("Hash_sum:");
		hash_sum.print_contents();
		printf("Count:");
		count.print_contents();
	}

	bool is_empty() const {
		return(key_sum.is_empty() && hash_sum.is_empty() && count.is_empty());
	}

	bool operator==( const multiIBLT_bucket& other ) const
	{
		return( key_sum == other.key_sum && hash_sum == other.hash_sum );
	}

	bool operator<( const multiIBLT_bucket& other ) const 
	{
		return( key_sum < other.key_sum || hash_sum < other.hash_sum );
	}
};

template <size_t n_parties = 2, typename key_type = uint32_t, size_t key_bits= 8*sizeof(key_type), typename hash_type = uint32_t>
class multiIBLT_bucket_extended: public multiIBLT_bucket<n_parties, key_type, key_bits, hash_type> {
  public:
  	typedef multiIBLT_bucket<n_parties, key_type, key_bits, hash_type> parent_bucket_type;
  	typedef multiIBLT_bucket_extended<n_parties, key_type, key_bits, hash_type> this_bucket_type;
	std::bitset<n_parties> has_key;
	multiIBLT_bucket_extended(): has_key(0) {}

	size_t size_in_bits() {
		//TODO: THEORETICAL
		return( parent_bucket_type::size_in_bits() + n_parties);
	}

	void serialize(file_sync::IBLT_bucket_extended& bucket) {
		file_sync::IBLT_bucket* basic_bucket = bucket.mutable_bucket();
		parent_bucket_type::serialize(*basic_bucket);
		for(size_t i = 0; i < n_parties; ++i) {
			bucket.add_has_key(has_key[i]);
		}
	}

	void deserialize(const file_sync::IBLT_bucket_extended& bucket) {
		parent_bucket_type::deserialize(bucket.bucket());
		for(size_t i = 0; i < n_parties; ++i) {
			has_key[i] = bucket.has_key(i);
		}
	}

	// void serialize(file_sync::IBLT_bucket& bucket) {
	// 	parent_bucket_type::serialize();
	// }

	// void deserialize(const file_sync::IBLT_bucket& bucket) {
	// 	parent_bucket_type::deserialize();
	// }

	void add(const this_bucket_type& counterparty_bucket) {
		parent_bucket_type::add(counterparty_bucket);
		has_key ^= counterparty_bucket.has_key;
	}

	// add a bucket corresponding to a specific index.
	// that bucket should only have that index bit potentially set
	void add(const this_bucket_type& counterparty_bucket, size_t index) {
		parent_bucket_type::add(counterparty_bucket);
		has_key[index] = has_key[index] ^ counterparty_bucket.has_key[0];
	}

	void add(const key_type& k, const hash_type& h) {
		parent_bucket_type::add(k, h);
		has_key[0] = ~has_key[0];
	}
	
	void remove(const this_bucket_type& counterparty_bucket) {
		parent_bucket_type::remove(counterparty_bucket);
		has_key ^= counterparty_bucket.has_key;
	}

	void remove(const this_bucket_type& counterparty_bucket, size_t index) {
		parent_bucket_type::remove(counterparty_bucket);
		has_key[index] = has_key[index] ^ counterparty_bucket.has_key[0];
	}

	void remove(const key_type& k, const hash_type& h) {
		parent_bucket_type::remove(k, h);
		has_key[0] = ~has_key[0];
	}

	void print_contents() const {
		parent_bucket_type::print_contents();
		printf("Bitset:");
		for(size_t i = 0; i < n_parties; ++i) {
			printf("%d", has_key[i]);
		}
		printf("\n");
	}
};

/**
Parameters:
	num_hashfns: number of hash functions (equivalent to k in the paper)
	hasher: type of hashfunction (should be able to hash keytype)
**/
template <size_t n_parties = 2, 
	typename key_type = uint32_t,
	size_t key_bits = 8*sizeof(key_type),
	typename hash_type = uint32_t, 
	typename bucket_type = multiIBLT_bucket<n_parties, key_type, key_bits, hash_type>,
//	typename bucket_type = multiIBLT_bucket_extended<n_parties, key_type, key_bits, hash_type>,
	//typename hasher = TabulationHashing<key_bits, hash_type> >
	typename hasher = MurmurHashing<key_bits, hash_type> >
class multiIBLT {
  public:
  	typedef multiIBLT<n_parties, key_type, key_bits, hash_type, bucket_type, hasher> this_iblt_type;
  	typedef std::vector<bucket_type> IBLT_type;
  	size_t num_buckets;
	size_t num_hashfns;
	size_t buckets_per_subIBLT;
  	std::vector<IBLT_type> subIBLTs;
  	hasher key_hasher;
  	std::vector<hasher> sub_hashers; 

	//seed num_hashfns different hashfunctions for each subIBLT
	multiIBLT(size_t bucket_count, size_t num_hashfns): 
							num_buckets(bucket_count), 
							num_hashfns(num_hashfns),
							subIBLTs(num_hashfns),
							sub_hashers(num_hashfns) {
		setup();
	}

	void setup() {
		while( num_buckets % num_hashfns != 0 && num_buckets == 0) {
			++num_buckets;
		}
		assert(num_buckets % num_hashfns == 0);
		buckets_per_subIBLT = num_buckets/num_hashfns;
		assert(buckets_per_subIBLT > 0 );
		key_hasher.set_seed(0);
		for(size_t i = 0; i < num_hashfns; ++i) {
			subIBLTs[i].resize(buckets_per_subIBLT);
			sub_hashers[i].set_seed(i+1); //separate seeds enough
		}

	}

	multiIBLT( const this_iblt_type& cp_IBLT ): 
						num_buckets(cp_IBLT.num_buckets),
						num_hashfns(cp_IBLT.num_hashfns),
						subIBLTs(num_hashfns),
						sub_hashers(num_hashfns) {
//		num_buckets = cp_IBLT.num_buckets;
//		num_hashfns = cp_IBLT.num_hashfns;
//		subIBLTs.resize(num_hashfns);
//		sub_hashers.resize(num_hashfns)
		setup();
		add(cp_IBLT);
	}

	size_t size_in_bits() {
		return( num_buckets * subIBLTs[0][0].size_in_bits() );
	}
	
	void serialize(file_sync::IBLT& iblt_serialized) {
		for(size_t i = 0; i < num_hashfns; ++i) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				file_sync::IBLT_bucket_extended* bucket = iblt_serialized.add_buckets();
				subIBLTs[i][j].serialize(*bucket);
			}
		}
	}

	void deserialize(file_sync::IBLT& iblt_serialized) {
		for(size_t i = 0; i < num_hashfns; ++i) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				subIBLTs[i][j].deserialize(iblt_serialized.buckets((buckets_per_subIBLT*i+j)));
			}
		}
	}

	void add(const this_iblt_type& counterparty) {
		assert( counterparty.buckets_per_subIBLT == buckets_per_subIBLT 
			&&  counterparty.num_hashfns == num_hashfns);
		for(size_t i = 0; i < num_hashfns; ++i) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				subIBLTs[i][j].add( counterparty.subIBLTs[i][j]);
			}
		}
	}

	void remove(const this_iblt_type& counterparty) {
		assert( counterparty.buckets_per_subIBLT == buckets_per_subIBLT 
			&&  counterparty.num_hashfns == num_hashfns);
		for(size_t i = 0; i < num_hashfns; ++i) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				subIBLTs[i][j].remove( counterparty.subIBLTs[i][j] );
			}
		}
	}
	
	void multiply(int mult_factor) {
		for(size_t i = 0; i < num_hashfns; ++i) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				subIBLTs[i][j].multiply(mult_factor);
			}
		}
	}

	void insert_keys(std::unordered_set<key_type>& keys) {
		for(auto it = keys.begin(); it!= keys.end(); ++it) {
			insert_key(*it);
		}
	}

	void insert_key(const key_type& key) {
		size_t bucket_index;
		hash_type hashval = key_hasher.hash(key);
		//std::cout << "Inserting key " << key << " with hash " << hashval << std::endl;
		for(size_t i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(key, i);
			subIBLTs[i][bucket_index].add( key, hashval);
		}
	}

	void remove_key(const key_type& key) {
		size_t bucket_index;
		hash_type hashval = key_hasher.hash(key);
		for(size_t i = 0; i < num_hashfns; ++i) {
			bucket_index = get_bucket_index(key, i);
			subIBLTs[i][bucket_index].remove( key, hashval);
		}
	}	

		//peels the keys from an IBLT, returning true upon success, false upon failure
	bool peel(std::unordered_set<key_type>& peeled_keys ) {
		std::deque<bucket_type> peelable_keys;
		bucket_type curr_bucket;

		//if we go through every entry and nothing is peelable, then we stop
		while( true ) {
			//first try to peel all the keys that can be peeled
			while( !peelable_keys.empty() ) {
				curr_bucket = peelable_keys.front();
				peelable_keys.pop_front();
				key_type peeled_key;
				curr_bucket.key_sum.extract_key(peeled_key);
				if( peeled_keys.find(peeled_key) == peeled_keys.end()) { // haven't peeled this key before
					peeled_keys.insert(peeled_key);
					peel_key( curr_bucket, peelable_keys );
				} else {
				}
			}

			//either every bucket has one key or more, in which case we failed,
			//or every bucket has zero keys, in which case we succeeded;
			if( !find_peelable_key(peelable_keys) ) {
				return( is_empty() );
				//printf("Peelable keys size is %d\n", peelable_keys.size());
			}
		}

		//can't reach this point
		assert(1 == 2);
		return false;
	}

	bool is_empty() {
		for(size_t i = 0; i < num_hashfns; ++i) {
			for(size_t j = 0; j < buckets_per_subIBLT; ++j) {
				if( !subIBLTs[i][j].is_empty()) {
					return false;
				}
			}
		}
		return true;
	}

	void filter_keys(std::unordered_map<key_type, std::vector<int> >& key_mapping,
					 std::unordered_set<key_type>& peeled_keys) {
		for(auto it = key_mapping.begin(); it != key_mapping.end(); ++it) {
			peeled_keys.insert(it->first);
		}
	}

	// returns whether was able to find peelable key
	// stores whether during search 
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
	
	void peel_key(bucket_type& peelable_bucket, std::deque<bucket_type>& peelable_keys) {
		size_t bucket_index;
		for(size_t i = 0; i < num_hashfns; ++i) {
			key_type buf;
			peelable_bucket.key_sum.extract_key(buf);
			bucket_index = get_bucket_index(buf, i);
			subIBLTs[i][bucket_index].remove(peelable_bucket);
			
		}
	}
	
	//TODO: handle case of removing non-existent keys
	//Will need to iterate from -nparties+1 to n_parties-1 (skipping 0)
	bool can_peel(bucket_type& curr_bucket) {
		int count = curr_bucket.count.get_contents();
		if( count == 0 ) {
			//printf("Current bucket count is: %d\n", curr_bucket.count.get_contents());
			return false;
		}
		
		hash_type expected_hash;
		key_type buf;
		
		if( curr_bucket.key_sum.can_divide_by( count )
			&& curr_bucket.hash_sum.can_divide_by( count )) {

			curr_bucket.key_sum.extract_key(buf);
			curr_bucket.hash_sum.extract_key(expected_hash);
			hash_type actual_hash = key_hasher.hash(buf);
			//std::cout << "Buffer: " << buf << ", Actual Hash: " << actual_hash << ", Expected Hash: " << expected_hash << std::endl;
			if( expected_hash == actual_hash) {
					// printf("Curr bucket count disagrees\n");
					// curr_bucket.print_contents();
				return true;
			}
		}
		return false;
	}

	//returns the bucket index of given key in given subIBLT
	uint32_t get_bucket_index(const key_type& key, size_t subIBLT) {
		assert( subIBLT >= 0 && subIBLT < num_hashfns );
		return sub_hashers[subIBLT].hash(key) % buckets_per_subIBLT;
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
