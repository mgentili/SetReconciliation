#include <stdint.h>
#include <cstdlib>
#include "basicIBLT.hpp"
#include "IBLT_helpers.hpp"

template <typename key_type>
void run_trial(int seed, int num_hashfns, int num_buckets, int num_keys) {
	srand(seed);
	basicIBLT<key_type> iblt(num_buckets, num_hashfns);
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

template <typename key_type>
void testXOR(int seed, int num_hashfns, int num_buckets, 
			 int num_shared_keys, int num_distinct_keys) {
	srand(seed);
	basicIBLT<key_type> Angel_IBLT(num_buckets, num_hashfns), Buffy_IBLT(num_buckets, num_hashfns);

	//need to ensure insert distinct keys
	std::unordered_set<key_type> shared_keys, Angel_keys, Buffy_keys, non_shared_keys;
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
	std::unordered_set<key_type> Angel_peeled;
	bool res1 = Angel_IBLT.peel(Angel_peeled);
	//Angel_IBLT.print_contents();
	if( !res1 )
		printf("Failed to peel with %d total, %d actual keys, %d slots\n",
			num_distinct_keys + num_shared_keys, num_distinct_keys, num_buckets);
	else {
		set_union(Angel_keys, Buffy_keys, non_shared_keys);
		checkResults<key_type>(Angel_peeled, non_shared_keys);
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
	simulateIBLT<uint64_t>(1 << 10, 1, true);
	//testXOR<int64_t>(0, 4, 1 << 5, 10, 10);
}