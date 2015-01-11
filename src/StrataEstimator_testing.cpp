#include "StrataEstimator.hpp"
#include "IBLT_helpers.hpp"

template <size_t n_parties, typename key_type = uint32_t>
void SimpleTest(size_t num_distinct_keys) {
	typedef keyGenerator<key_type, 8*sizeof(key_type)> gen_type;
	typedef StrataEstimator<n_parties, key_type> est_type;
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

	for(size_t i = 1; i < n_parties; ++i) {
		estimators[0].add(estimators[i], i);
	}
	size_t diff_estimate = estimators[0].set_diff_estimate();
	std::cout << "Actual difference is " << num_distinct_keys << "while estimated difference is " << diff_estimate;
	std::cout << "Overhead is " << ((double) diff_estimate)/num_distinct_keys << std::endl;
}

void ExtendedTesting() {
	for(int i = 10; i <= 10000000; i*=10) {
		SimpleTest<2>(i);
	}

	for(int i = 10; i <= 10000000; i*=10) {
		SimpleTest<3>(i);
	}
}

void TestNumTrailingZeroes() {
	typedef uint32_t key_type;
	const size_t n_parties = 2;
	typedef StrataEstimator<n_parties, key_type> est_type;
	est_type estimator;
	assert( estimator.num_trailing_zeroes(0) == 32);
	assert( estimator.num_trailing_zeroes(1) == 0);
	assert( estimator.num_trailing_zeroes(2) == 1);
	assert( estimator.num_trailing_zeroes(3) == 0);
	assert( estimator.num_trailing_zeroes(32) == 5);
}
int main() {
	TestNumTrailingZeroes();
	ExtendedTesting();
	return 1;
}