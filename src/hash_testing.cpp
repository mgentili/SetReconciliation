#include "IBLT_helpers.hpp"
#include "hash_util.hpp"
static inline uint64_t hashBits( uint64_t hash, int bits ) {
	return( ((1UL << bits) - 1) & hash );
}

void hashCollisionTest(int num_bits, int num_trials) {
	typedef uint64_t key_type;
	keyHandler<key_type> kh;
	size_t num_keys = 1 << (num_bits/2);
	std::unordered_set<key_type> keys, hashes;
	kh.generate_distinct_keys( num_keys, keys);
	
	int successes = 0;
	for(int trial = 0; trial < num_trials; ++trial) {
		std::unordered_set<key_type> hashes;
		for(auto it = keys.begin(); it != keys.end(); ++it) {
			key_type hash = HashUtil::MurmurHash64A( &(*it), sizeof(key_type), trial); 
			hash = hashBits( hash, num_bits );
			hashes.insert(hash);
		}
		successes += (hashes.size() == num_keys );
		std::cout << "Success rate: " << (double) successes/(trial+1) << std::endl;
	}

}


int main() {
	int num_bits = 41;
	int num_trials = 1000;
	hashCollisionTest(num_bits, num_trials);

	return 1;

}
