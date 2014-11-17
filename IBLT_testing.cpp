#include <set>
#include <stdint.h>
#include <assert.h>
#include <cstdlib>
#include "basicIBLT.hpp"

//--TESTING CODE--

/**  checkResults sorts the output of two vectors and ensures they are equal **/
template <typename key_type>
void checkResults(std::set<key_type>& expected, std::set<key_type>& actual) {
	//std::sort( expected.begin(), expected.end() );
	//std::sort( actual.begin(), actual.end() );
	/*printf("Expected:\n");
	for(auto it = expected.begin(); it != expected.end(); ++it) {
		printf("%ld ", *it);
	}
	printf("\n");
	printf("Actual:\n");
	for(auto it = actual.begin(); it != actual.end(); ++it) {
		printf("%ld ", *it);
	}*/
	if( expected.size() != actual.size() ) {
		printf("Expected: %zu, Actual: %zu\n", expected.size(), actual.size());
		return;
	}

	//assert(expected.size() == actual.size() );
	auto it1 = actual.begin();
	auto it2 = expected.begin();
	for(; it1 != actual.end(); ++it1, ++it2 ) {
		assert( *it1 == *it2 );
	}
}

template <typename key_type>
void generate_distinct_keys(int num_keys, std::set<key_type>& keys) {
	int num_inserted = 0;
	key_type new_key;
	while( num_inserted != num_keys) {
		new_key = (key_type) rand();
		if( keys.insert( new_key ).second ) {
			//printf("Inserting key %ld\n", new_key);
			++num_inserted;
		}
	}
}

template <typename key_type>
void generate_keys_distict_from(int num_keys, std::set<key_type>& taboo_keys, std::set<key_type>& res) {
	int num_inserted = 0;
	key_type new_key;
	while( num_inserted != num_keys) {
		new_key = (key_type) rand();
		if( taboo_keys.find(new_key) != taboo_keys.end()
			&& res.insert(new_key).second ) {
			++num_inserted;
		}
	}
}

template <typename key_type>
void set_union(std::set<key_type>& keys1, std::set<key_type>& keys2, std::set<key_type>& final_set) {
	for( auto it = keys1.begin(); it != keys1.end(); ++it) {
		final_set.insert(*it);
	}
	for( auto it = keys2.begin(); it != keys2.end(); ++it) {
		final_set.insert(*it);
	}
}

template <typename key_type>
void generate_sample_keys(int num_shared_keys, int num_distinct_keys,
							std::set<key_type>& shared_keys,
							std::set<key_type>& a_keys,
							std::set<key_type>& b_keys) {
	std::set<key_type> all_keys;
	generate_distinct_keys<key_type>(num_shared_keys + 2*num_distinct_keys, all_keys);
	int count = 0;
	auto it = all_keys.begin();
	for(; it != all_keys.end(); ++it, ++count) {
		if( count < num_shared_keys ) {
			shared_keys.insert(*it);
		} else if( count < num_shared_keys + num_distinct_keys) {
			a_keys.insert(*it);
		} else {
			b_keys.insert(*it);
		}
	}
}

template <typename key_type>
void run_trial(int seed, int num_hashfns, int num_buckets, int num_keys) {
	srand(seed);
	IBLT<key_type> iblt(num_buckets, num_hashfns);
	//need to ensure insert distinct keys
	std::set<key_type> keys_to_insert;
	generate_distinct_keys<key_type>(num_keys, keys_to_insert);
	for( auto it = keys_to_insert.begin(); it != keys_to_insert.end(); ++it) {
		iblt.insert_key(*it);
	}
	//printf("Before\n");
	//iblt.print_contents();
	std::set<key_type> peeled_keys;
	bool res = iblt.peel(peeled_keys);
	//printf("After\n");
    //iblt.print_contents();
	if( !res )
		printf("Failed to peel with %d keys, %d slots\n", num_keys, num_buckets);
	else {
		checkResults(keys_to_insert, peeled_keys);
		printf("Successfully peeled %d keys, %d slots\n", num_keys, num_buckets);
	}
}

template <typename key_type>
void testXOR(int seed, int num_hashfns, int num_buckets, 
			 int num_shared_keys, int num_distinct_keys) {
	srand(seed);
	IBLT<key_type> Angel_IBLT(num_buckets, num_hashfns), Buffy_IBLT(num_buckets, num_hashfns);

	//need to ensure insert distinct keys
	std::set<key_type> shared_keys, Angel_keys, Buffy_keys, non_shared_keys;
	generate_sample_keys<key_type>( num_shared_keys, num_distinct_keys, shared_keys, Angel_keys, Buffy_keys);
	for( auto it = shared_keys.begin(); it != shared_keys.end(); ++it) {
		Angel_IBLT.insert_key( *it );
		Buffy_IBLT.insert_key( *it ); 
	}
	for( auto it = Angel_keys.begin(); it != Angel_keys.end(); ++it) {
		Angel_IBLT.insert_key( *it );
	}
	for( auto it = Buffy_keys.begin(); it != Buffy_keys.end(); ++it) {
		Buffy_IBLT.insert_key( *it );
	}

	Angel_IBLT.XOR(Buffy_IBLT);
	//printf("Angel before\n");
	//Angel_IBLT.print_contents();
	std::set<key_type> Angel_peeled;
	bool res1 = Angel_IBLT.peel(Angel_peeled);
	//Angel_IBLT.print_contents();
	if( !res1 )
		printf("Failed to peel with %d total, %d actual keys, %d slots\n",
			num_distinct_keys + num_shared_keys, num_distinct_keys, num_buckets);
	else {
		set_union(Angel_keys, Buffy_keys, non_shared_keys);
		checkResults(Angel_peeled, non_shared_keys);
		printf("Successfully peeled with %d total, %d actual keys, %d slots\n",
			num_distinct_keys + num_shared_keys, num_distinct_keys, num_buckets);
	}
}

/**
simulateIBLT tries inserting varying numbers of keys into the IBLT
and subsequently tries to peel. this helps in determining what the threshold
load is
**/
template <typename key_type>
void simulateIBLT(int num_buckets, int num_trials, bool one_IBLT ) {
	const int num_hfs = 4;
	//for(int num_keys = 20; num_keys < num_buckets; num_keys*= 1.05) {
	for(int num_keys = 1; num_keys < num_buckets; num_keys+= 1) {
		for(int trial = 0; trial < num_trials; ++trial ) {
			int seed = trial* (1 << 25) + num_buckets;
			if( one_IBLT)
				run_trial<key_type>(seed, num_hfs, num_buckets, num_keys);
			else
				testXOR<key_type>(seed, num_hfs, num_buckets, num_keys/2, num_keys/2);
		}
	}
}

int main() {
	simulateIBLT<int64_t>(1 << 10, 10, true);
	//testXOR<int64_t>(0, 4, 1 << 5, 10, 10);
}