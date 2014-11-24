#include <stdint.h>
#include <assert.h>
#include <cstdlib>
#include "multiIBLT.hpp"
#include "IBLT_helpers.hpp"
#include <iostream>

//--TESTING CODE--
/*
template <typename key_type>
void run_trial(int seed, int num_hashfns, int num_buckets, int num_keys) {
	srand(seed);
	multiIBLT<key_type> iblt(num_buckets, num_hashfns);
	//need to ensure insert distinct keys
	std::unordered_set<key_type> keys_to_insert;
	generate_distinct_keys<key_type>(num_keys, keys_to_insert);
	for( auto it = keys_to_insert.begin(); it != keys_to_insert.end(); ++it) {
		iblt.insert_key(*it);
	}
	//printf("Before\n");
	//iblt.print_contents();
	std::unordered_set<key_type> peeled_keys;
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
*/
template <typename key_type>
void testAdd(int seed, int num_hashfns, int num_buckets, 
			 int num_shared_keys, int num_distinct_keys) {
	srand(seed);
	multiIBLT<3, key_type> Angel_IBLT(num_buckets, num_hashfns), Buffy_IBLT(num_buckets, num_hashfns);

	//need to ensure insert distinct keys
	std::unordered_set<key_type> shared_keys, Angel_keys, Buffy_keys, non_shared_keys;
	generate_sample_keys<key_type>( num_shared_keys, num_distinct_keys, shared_keys, Angel_keys, Buffy_keys);
	for( auto it = shared_keys.begin(); it != shared_keys.end(); ++it) {
		std::cout << "Inserting to both: " << *it << std::endl;
		Angel_IBLT.insert_key( *it );
		Buffy_IBLT.insert_key( *it ); 
	}
	for( auto it = Angel_keys.begin(); it != Angel_keys.end(); ++it) {
		std::cout << "Inserting to Angel: " << *it << std::endl;
		Angel_IBLT.insert_key( *it );
	}
	for( auto it = Buffy_keys.begin(); it != Buffy_keys.end(); ++it) {
		Buffy_IBLT.insert_key( *it );
	}
	//printf("Angel before\n");
	//Angel_IBLT.print_contents();
	//Angel_IBLT.add(Buffy_IBLT);
	//Angel_IBLT.print_contents();
	std::unordered_set<key_type> Angel_peeled;
	bool res1 = Angel_IBLT.peel(Angel_peeled);
	//printf("Angel after\n");
	//Angel_IBLT.print_contents();
	//std::sort(ordered_keys.begin(), ordered_keys.end() );
	printf("Peeled a total of %zu keys", Angel_peeled.size());
	for(auto it = Angel_peeled.begin(); it != Angel_peeled.end(); ++it) {
		printf("Peeled key %lu\n", *it);
	}

	if( !res1 )
		printf("Failed to peel with %d total, %d actual keys, %d slots\n",
			num_distinct_keys + num_shared_keys, num_distinct_keys, num_buckets);
	else {
		set_union(Angel_keys, Angel_keys, non_shared_keys);
		//checkResults<key_type>(Angel_peeled, non_shared_keys);
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
void simulateIBLT(int num_buckets, int num_trials, bool deletes ) {
	const int num_hfs = 4;
	//for(int num_keys = 20; num_keys < num_buckets; num_keys*= 1.05) {
	for(int num_keys = 2; num_keys < num_buckets; num_keys*= 2) {
		for(int trial = 0; trial < num_trials; ++trial ) {
			int seed = trial* (1 << 25) + num_buckets;
			/*if( !deletes )
				run_trial<key_type>(seed, num_hfs, num_buckets, num_keys);
			else*/
			testAdd<key_type>(seed, num_hfs, num_buckets, num_keys/2, num_keys/2);
		}
	}
}

int main() {
	int num_buckets = 1 << 8;
	int num_trials = 1;
	bool deletes = false;
	int num_hashfns = 4;
	int seed = 0;
	int num_shared_keys = 1;
	int num_distinct_keys = 0;
	simulateIBLT<uint64_t>(num_buckets, num_trials, deletes);
	//testAdd<uint64_t>(seed, num_hashfns, num_buckets, num_shared_keys, num_distinct_keys);
}