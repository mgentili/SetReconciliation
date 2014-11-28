#include <stdint.h>
#include <assert.h>
#include <cstdlib>
#include "multiIBLT.hpp"
#include "IBLT_helpers.hpp"
#include <iostream>

//--TESTING CODE--

template <typename key_type, int key_bits = 8*sizeof(key_type)>
void randomizedTesting(int num_hashfns, int num_buckets) {
	const int n_parties = 3;
	typedef multiIBLT<n_parties, key_type, key_bits> iblt_type; 
	typedef keyGenerator<key_type, key_bits> gen_type;
	keyHandler<key_type, gen_type> keyhand;
	std::vector<iblt_type* > iblts(n_parties);
	for(int i = 0 ; i < n_parties; ++i) {
		iblts[i] = new multiIBLT<n_parties, key_type>(num_buckets, num_hashfns);
	}

	//generate_distinct_keys()
}

template <typename key_type, int key_bits = 8*sizeof(key_type)>
void testAdd(int seed, int num_hashfns, int num_buckets, 
			 int num_shared_keys, int num_distinct_keys) {
	srand(seed);
	const int n_parties = 2;
	typedef multiIBLT<n_parties, key_type, key_bits> iblt_type;
	typedef keyGenerator<key_type, key_bits> gen_type;
	keyHandler<key_type, gen_type> keyhand;
	std::vector<iblt_type* > iblts(n_parties);
	for(int i = 0 ; i < n_parties; ++i) {
		iblts[i] = new iblt_type(num_buckets, num_hashfns);
	}

	//need to ensure insert distinct keys
	std::unordered_set<key_type> shared_keys, distinct_keys;
	std::vector<std::unordered_set<key_type> > indiv_keys(n_parties);

	keyhand.generate_sample_keys( num_shared_keys, num_distinct_keys, shared_keys, indiv_keys);
	for( auto it = shared_keys.begin(); it != shared_keys.end(); ++it) {
		//IBLT_DEBUG("Inserting to all key " << *it << std::endl);
		for(uint i = 0; i < iblts.size(); ++i) {
			iblts[i]->insert_key( *it );
		}
	}
	for( uint i = 0; i < indiv_keys.size(); ++i ) {
		for(auto it = indiv_keys[i].begin(); it != indiv_keys[i].end(); ++it) {
			//IBLT_DEBUG("Inserting to IBLT " << i << "key: " << *it << std::endl);
			iblts[i]->insert_key(*it);
		}
		//IBLT_DEBUG("Printing contents of IBLT" << i << std::endl);
		//iblts[i]->print_contents();
	}

	iblt_type resIBLT(num_buckets, num_hashfns);
	for( uint i = 0; i < iblts.size(); ++i) {
		resIBLT.add(*iblts[i]);
	}
	
	//IBLT_DEBUG("Printing contents of Result IBLT before peeling" << std::endl);
	//resIBLT.print_contents();
	std::unordered_set<key_type> peeled_keys;
	bool res1 = resIBLT.peel(peeled_keys);
	//IBLT_DEBUG("Printing contents of Result IBLT after peeling" << std::endl);
	//resIBLT.print_contents();
	
	if( !res1 )
		printf("Failed to peel\n");
	else {
		set_union(indiv_keys, distinct_keys);
		checkResults<key_type>(distinct_keys, peeled_keys);
		printf("Successfully peeled\n");
	}
	printf("%d parties, %d shared, %d distinct keys, %d slots per IBLT, %lu keys in result IBLT\n", 
			n_parties, num_shared_keys, num_distinct_keys, num_buckets, peeled_keys.size());
	for(uint i = 0; i < iblts.size(); ++i) {
		delete iblts[i];
	}
}

/**
simulateIBLT tries inserting varying numbers of keys into the IBLT
and subsequently tries to peel. this helps in determining what the threshold
load is
**/
template <typename key_type, int key_bits = 8*sizeof(key_type)>
void simulateIBLT(int num_buckets, int num_hashfns, int num_trials, double pctshared) {
	const int num_hfs = 4;
	//for(int num_keys = 20; num_keys < num_buckets; num_keys*= 1.05) {
	for(int num_keys = 5; num_keys < num_buckets; num_keys*=1.2) {
		for(int trial = 0; trial < num_trials; ++trial ) {
			int seed = trial* (1 << 25) + num_buckets;
			/*if( !deletes )
				run_trial<key_type>(seed, num_hfs, num_buckets, num_keys);
			else*/
			testAdd<key_type, key_bits>(seed, num_hfs, num_buckets, num_keys*(pctshared), num_keys*(1-pctshared));
		}
	}
}

int main() {
	int num_buckets = 1 << 15;
	int num_trials = 1;
	int num_hashfns = 4;
	//int seed = 0;
	double pctshared = 0.5;
	//int num_shared_keys = 1;
	//int num_distinct_keys = 0;
	//simulateIBLT<uint64_t>(num_buckets, num_hashfns, num_trials, pctshared);
	simulateIBLT<std::string, 64>(num_buckets, num_hashfns, num_trials, pctshared);
	//testAdd<uint64_t>(seed, num_hashfns, num_buckets, num_shared_keys, num_distinct_keys);
}