#include "StrataEstimator.hpp"

#include "IBLT_helpers.hpp"

template <typename key_type = uint32_t>
void SimpleTest(size_t num_distinct_keys) {
	const size_t n_parties = 2;
	typedef keyGenerator<key_type, 8*sizeof(key_type)> gen_type;
	typedef StrataEstimator<key_type> est_type;
	std::vector<est_type> estimators(n_parties);
	std::unordered_set<key_type> shared_keys;
	std::vector<std::unordered_set<key_type> > key_sets(n_parties);
	keyHandler<key_type, gen_type> keyhand;

	keyhand.generate_sample_keys(0, num_distinct_keys/n_parties, shared_keys, key_sets);

	for(size_t i = 0; i < key_sets.size(); ++i) {
		for(auto it = key_sets[i].begin(); it != key_sets[i].end(); ++it) {
			estimators[i].insert_key(*it);
		}
	}

	size_t diff_estimate = estimators[0].estimate_diff(estimators[1]);
	std::cout << "Actual difference is " << num_distinct_keys 
              << "while estimated difference is " << diff_estimate;
	std::cout << "Overhead is " << ((double) diff_estimate)/num_distinct_keys << std::endl;
}

void ExtendedTesting() {
	for(int i = 10; i <= 10000000; i*=10) {
		SimpleTest<uint32_t>(i);
	}
}

void TestNumTrailingZeroes() {
	typedef uint32_t key_type;
	typedef StrataEstimator<key_type> est_type;
	assert( est_type::num_trailing_zeroes(0) == 32);
	assert( est_type::num_trailing_zeroes(1) == 0);
	assert( est_type::num_trailing_zeroes(2) == 1);
	assert( est_type::num_trailing_zeroes(3) == 0);
	assert( est_type::num_trailing_zeroes(32) == 5);
}

void TestHashDistribution( int total_numbers ) {
	typedef  uint32_t key_type;
	typedef StrataEstimator<key_type> est_type;
	std::vector<int> counts(32, 0);
	int i = 0;
	for(; i < total_numbers; ++i) {
		++counts[est_type::num_trailing_zeroes(HashUtil::MurmurHash64A ( &i, sizeof(int), 0 ))];
	}
	for(int i = 0; i < 32; ++i) {
		std::cout << "Percent of numbers with " << i 
                  << "zeroes is: " << (double) counts[i]/total_numbers 
                  << "vs theoretical of " << (double) 1/(1 << (i+1)) << std::endl;
	}
}

int main() {
	TestNumTrailingZeroes();
	TestHashDistribution(1 << 20);
	ExtendedTesting();
	return 1;
}
