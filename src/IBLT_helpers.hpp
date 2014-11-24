#ifndef IBLT_HELPERS
#define IBLT_HELPERS

#include <unordered_set>
#include <cstdlib>
#include <assert.h>

/**  checkResults asserts that two sets contain the same information**/
template <typename key_type>
void checkResults(std::unordered_set<key_type>& expected, std::unordered_set<key_type>& actual) {
	if( expected.size() != actual.size() ) {
		printf("Expected: %zu, Actual: %zu\n", expected.size(), actual.size());
		return;
	}

	for(auto it1 = expected.begin(); it1 != expected.end(); ++it1 ) {
		assert( actual.find(*it1) != actual.end() );
	}
}

template <typename key_type>
void generate_distinct_keys(int num_keys, std::unordered_set<key_type>& keys) {
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
void generate_keys_distict_from(int num_keys, std::unordered_set<key_type>& taboo_keys, std::unordered_set<key_type>& res) {
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
void set_union(std::unordered_set<key_type>& keys1, std::unordered_set<key_type>& keys2, std::unordered_set<key_type>& final_set) {
	for( auto it = keys1.begin(); it != keys1.end(); ++it) {
		final_set.insert(*it);
	}
	for( auto it = keys2.begin(); it != keys2.end(); ++it) {
		final_set.insert(*it);
	}
}

template <typename key_type>
void set_union(std::vector<key_type>& key_sets, std::unordered_set<key_type>& final_set) {
	for(auto it1 = key_sets.begin(); it1 != key_sets.end(); ++it1) {
		for(auto it2 = (*it1).begin(); it2 != (*it2).end(); ++it2) {
			final_set.insert(*it2);
		}
	}
}

template <typename key_type>
void generate_sample_keys(int num_shared_keys, int num_distinct_keys,
							std::unordered_set<key_type>& shared_keys,
							std::unordered_set<key_type>& a_keys,
							std::unordered_set<key_type>& b_keys) {
	std::unordered_set<key_type> all_keys;
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
void generate_sample_keys(int num_shared_keys, int num_distinct_keys,
							std::unordered_set<key_type>& shared_keys,
							std::vector<std::unordered_set<key_type> > key_sets) {
	std::unordered_set<key_type> all_keys;
	generate_distinct_keys<key_type>(num_shared_keys + key_sets.size()*num_distinct_keys, all_keys);
	
	auto all_key_it = all_keys.begin();
	for(int count = 0; count < num_shared_keys; ++all_key_it, ++count) {
		shared_keys.insert(*all_key_it);
	}

	for(auto it1 = key_sets.begin(); it1 != key_sets.end(); ++it1) {
		for(int count = 0; count < num_distinct_keys; ++all_key_it, ++count) {
			(*it1).insert(*all_key_it);
		}
	}
}

#endif