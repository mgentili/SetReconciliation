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
	}

	void check_results_simple() {
		for( uint i = 0; i < iblts.size(); ++i) {
			resIBLT.add(*iblts[i]);
		}
		// std::cout << "Before peeling: " << std::endl;
		// resIBLT.print_contents();
		//get keys that aren't in all IBLTs
		std::unordered_set<key_type> distinct_keys;
		keyhand.set_difference(n_parties, key_assignments, distinct_keys);

		std::unordered_set<key_type> peeled_keys;
		bool res1 = resIBLT.peel(peeled_keys);
		// std::cout << "After peeling: " << std::endl;
		// resIBLT.print_contents();
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

template <typename key_type = uint64_t, int key_bits = 8*sizeof(key_type)>
void simulateTwoParty(int num_buckets, int num_keys) {
	const int num_trials = 10;
	const int num_hashfns = 4;
	for(int i = 1; i < num_trials; ++i) {
	    IBLT_tester<2, key_type, key_bits> tester(num_keys, num_buckets, num_hashfns);
	    double insert_prob = i/((double) num_trials);
    	tester.random_testing(insert_prob);
    }
} 

template <typename key_type = uint64_t, int key_bits = 8*sizeof(key_type)>
void simulateThreeParty(int num_buckets, int num_keys) {
	const int num_trials = 10;
	const int num_hashfns = 4;
	for(int i = 1; i < num_trials; ++i) {
	    IBLT_tester<3, key_type, key_bits> tester(num_keys, num_buckets, num_hashfns);
	    double insert_prob = i/((double) num_trials);
    	tester.random_testing(insert_prob);
    }
} 

int main() {
	const int num_buckets = 1 << 10;
	const int num_keys = 1 << 11;
	simulateTwoParty<uint32_t>(num_buckets, num_keys);
	//simulateThreeParty<uint32_t>(num_buckets, num_keys);
	//simulateThreeParty<std::string, 64>(num_buckets, num_keys);
	simulateTwoParty<std::string, 320>(num_buckets, num_keys);
	//testAdd<std::string, 320>(0, 4, num_buckets, 0, 1);
}
