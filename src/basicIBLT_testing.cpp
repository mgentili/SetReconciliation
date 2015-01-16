#include <stdint.h>
#include <cstdlib>
#include "basicIBLT.hpp"
#include "IBLT_helpers.hpp"

template <typename key_type, typename hash_type>
void run_trial(int seed, int num_hashfns, int num_buckets, int num_keys) {
	keyHandler<key_type> keyhand(seed);
	basicIBLT<key_type, hash_type> iblt(num_buckets, num_hashfns);
	
	//need to ensure insert distinct keys
	std::unordered_set<key_type> keys_to_insert;
	keyhand.generate_distinct_keys(num_keys, keys_to_insert);
	for( auto it = keys_to_insert.begin(); it != keys_to_insert.end(); ++it) {
		iblt.insert_key(*it);
	}
	std::unordered_set<key_type> peeled_keys;
	bool res = iblt.peel(peeled_keys);
	if( !res )
		printf("Failed to peel with %d keys, %d slots\n", num_keys, num_buckets);
	else {
		checkResults<key_type>(keys_to_insert, peeled_keys);
		printf("Successfully peeled %d keys, %d slots\n", num_keys, num_buckets);
	}
}

template <typename key_type, typename hash_type>
void testXOR(int seed, int num_hashfns, int num_buckets, 
			 int num_shared_keys, int num_distinct_keys) {
	typedef basicIBLT<key_type, hash_type> iblt_type;
	const size_t n_parties = 2;
	keyHandler<key_type> keyhand(seed);
	std::vector<iblt_type* > iblts(n_parties);
	for(size_t i = 0; i < n_parties; ++i) {
		iblts[i] = new iblt_type(num_buckets, num_hashfns);
	}
	//need to ensure insert distinct keys
	std::unordered_set<key_type> shared_keys, distinct_keys;
	std::vector<std::unordered_set<key_type> > indiv_keys(n_parties);

	keyhand.generate_sample_keys( num_shared_keys, num_distinct_keys, shared_keys, indiv_keys);
	for( auto it = shared_keys.begin(); it != shared_keys.end(); ++it) {
		//std::cout << "Inserting to all key " << *it << std::endl;
		for(uint i = 0; i < iblts.size(); ++i) {
			iblts[i]->insert_key( *it );
		}
	}
	for( uint i = 0; i < indiv_keys.size(); ++i ) {
		for(auto it = indiv_keys[i].begin(); it != indiv_keys[i].end(); ++it) {
			//std::cout << "Inserting to IBLT " << i << "key: " << *it << std::endl;
			iblts[i]->insert_key(*it);
		}
		//printf("Printing contents of IBLT %d\n", i);
		//iblts[i]->print_contents();
	}

	iblts[0]->XOR(*iblts[1]);
	std::unordered_set<key_type> my_peeled_keys, cp_peeled_keys;
	bool res1 = iblts[0]->peel(my_peeled_keys, cp_peeled_keys);
	size_t total_peeled = my_peeled_keys.size() + cp_peeled_keys.size();
	if( !res1 ) {
		total_peeled = total_peeled;
	//	std::cout << "Failed to Peel: " << total_peeled << " out of " << 2*num_distinct_keys << std::endl;
	} else {
		keyhand.set_union(indiv_keys, distinct_keys);
		checkResults<key_type>(my_peeled_keys, indiv_keys[0]);
		checkResults<key_type>(cp_peeled_keys, indiv_keys[1]);
	//	std::cout << "Successfully peeled" << std::endl;
	}
//	std::cout << n_parties << " parties with " 
//		  <<  num_shared_keys << " shared keys and " 
//		  << num_distinct_keys << " distinct keys "
//		  << num_buckets << " buckets per IBLT" << std::endl;
	std::cout << "percent_peeled: " << (double) total_peeled / (2*num_distinct_keys) 
		  << ", fill_ratio: " << (double) 2*num_distinct_keys/num_buckets << std::endl;
	for(uint i = 0; i < iblts.size(); ++i) {
		delete iblts[i];
	}
}

/**
simulateIBLT tries inserting varying numbers of keys into the IBLT
and subsequently tries to peel. this helps in determining what the threshold
load is
**/
template <typename key_type, typename hash_type>
void simulateIBLT(int num_buckets, int num_trials, bool one_IBLT ) {
	const int num_hfs = 4;
	//for(int num_keys = 20; num_keys < num_buckets; num_keys*= 1.05) {
	for(int num_keys = 1; num_keys < num_buckets; num_keys+= 1) {
		for(int trial = 0; trial < num_trials; ++trial ) {
			int seed = trial* (1 << 25) + num_buckets;
			if( one_IBLT)
				run_trial<key_type, hash_type>(seed, num_hfs, num_buckets, num_keys);
			else
				testXOR<key_type, hash_type>(seed, num_hfs, num_buckets, num_keys/2, num_keys/2);
		}
	}
}

template <typename key_type, typename hash_type>
void peelingRatio(int num_buckets, int num_trials) {
	const int num_hfs = 4;
	for(int num_keys = num_buckets/2; num_keys < 100*num_buckets; num_keys += (num_buckets/2) ) {
		for(int trial = 0; trial < num_trials; ++trial) {
			int seed = trial* (1 << 20 ) + num_keys;
			testXOR<key_type, hash_type>(seed, num_hfs, num_buckets, 0, num_keys/2);
		}
	}
}

int main() {
	peelingRatio<uint32_t, uint32_t>(1 << 12, 5);
	//testXOR<uint32_t, uint32_t>(0, 4, 80, 5, 5);
	//simulateIBLT<uint32_t, uint32_t>(1 << 10, 1, false);
	//testXOR<int64_t>(0, 4, 1 << 5, 10, 10);
}
