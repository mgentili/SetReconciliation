#include <stdint.h>
#include <assert.h>
#include <cstdlib>
#include "multiIBLT.hpp"
#include "IBLT_helpers.hpp"
#include <iostream>
#include <unordered_map>

//--TESTING CODE--

template <int n_parties, typename key_type, int key_bits = 8*sizeof(key_type)> 
class IBLT_tester {
  public:
  	typedef multiIBLT<n_parties, key_type, key_bits> iblt_type; 
	typedef keyGenerator<key_type, key_bits> gen_type;

	IBLT_tester(int num_keys, int num_buckets, int num_hashfns):
		num_keys(num_keys), num_buckets(num_buckets), num_hashfns(num_hashfns),
		iblts(n_parties), resIBLT(num_buckets, num_hashfns) {
		for(uint i = 0 ; i < iblts.size(); ++i) {
			iblts[i] = new iblt_type(num_buckets, num_hashfns);
		}
	}

	~IBLT_tester() {
		for(uint i = 0; i < iblts.size(); ++i) {
			delete iblts[i];
		}
	}

	/** inserts each key with insert_prob into each IBLT, then ensures that all keys
	 ** that aren't in all IBLTs are peeled properly and appropriates the keys accordingly
	 **/
	void random_testing(double insert_prob) {
		keyhand.assign_keys(insert_prob, n_parties, num_keys, key_assignments);

		//insert keys
		for(auto it1 = key_assignments.begin(); it1 != key_assignments.end(); ++it1) {
			for(auto it2 = (it1->second).begin(); it2 != (it1->second).end(); ++it2) {
				iblts[*it2]->insert_key(it1->first);
			}
		}
		if( n_parties == 2) {
			check_results_simple(); //using current method, two party cannot determine allocation
		} else {
			check_results();
		}
	}

	void check_results_simple() {
		for( uint i = 0; i < iblts.size(); ++i) {
			resIBLT.add(*iblts[i], i);
		}
		
		//get keys that aren't in all IBLTs
		std::unordered_set<key_type> distinct_keys;
		keyhand.set_difference(n_parties, key_assignments, distinct_keys);

		std::unordered_set<key_type> peeled_keys;
		bool res1 = resIBLT.peel(peeled_keys);
		std::cout << "Distinct keys size: " << distinct_keys.size() << " Peeled keys size: " << peeled_keys.size() << std::endl;

		if( !res1 )
			printf("Failed to peel\n");
		else {
			checkResults<key_type>(distinct_keys, peeled_keys);
			printf("Successfully peeled\n");
		}
		printf("%d parties, %d slots per IBLT, %lu keys in result IBLT\n", 
				n_parties, num_buckets, peeled_keys.size());
	}

	void check_results() {
		//add all iblts
		for( uint i = 0; i < iblts.size(); ++i) {
			resIBLT.add(*iblts[i], i);
		}
		
		//get keys that aren't in all IBLTs
		std::unordered_map<key_type, std::vector<int> > distinct_keys;
		keyhand.set_difference(n_parties, key_assignments, distinct_keys);

		std::unordered_map<key_type, std::vector<int> > peeled_keys;
		bool res1 = resIBLT.peel(peeled_keys);
		std::cout << "Distinct keys size: " << distinct_keys.size() << " Peeled keys size: " << peeled_keys.size() << std::endl;

		if( !res1 )
			printf("Failed to peel\n");
		else {
			checkResults<key_type>(distinct_keys, peeled_keys);
			printf("Successfully peeled\n");
		}
		printf("%d parties, %d slots per IBLT, %lu keys in result IBLT\n", 
				n_parties, num_buckets, peeled_keys.size());
	} 

  private:
  	int num_keys;
  	int num_buckets;
  	int num_hashfns;
  	keyHandler<key_type, gen_type> keyhand;
  	std::vector<iblt_type* > iblts;
  	iblt_type resIBLT;
  	std::unordered_map<key_type, std::vector<int> > key_assignments;
};


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
		keyhand.set_union(indiv_keys, distinct_keys);
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

template <typename key_type = uint64_t, int key_bits = 8*sizeof(key_type)>
void simulateTwoParty() {
	const int num_buckets = 1 << 20;
	const int num_trials = 10;
	const int num_hashfns = 4;
	const int num_keys = 1000000;
	for(int i = 1; i < num_trials; ++i) {
	    IBLT_tester<2, key_type, key_bits> tester(num_keys, num_buckets, num_hashfns);
	    double insert_prob = i/((double) num_trials);
    	tester.random_testing(insert_prob);
    }
} 

template <typename key_type = uint64_t, int key_bits = 8*sizeof(key_type)>
void simulateThreeParty() {
	const int num_buckets = 1 << 15;
	const int num_trials = 10;
	const int num_hashfns = 4;
	const int num_keys = 40000;
	for(int i = 1; i < num_trials; ++i) {
	    IBLT_tester<3, key_type, key_bits> tester(num_keys, num_buckets, num_hashfns);
	    double insert_prob = i/((double) num_trials);
    	tester.random_testing(insert_prob);
    }
} 

int main() {
	simulateTwoParty<uint32_t>();
	//simulateThreeParty<uint32_t>();
	//simulateThreeParty<std::string, 64>();
}