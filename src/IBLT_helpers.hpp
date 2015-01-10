#ifndef _IBLT_HELPERS
#define _IBLT_HELPERS

#include <unordered_set>
#include <cstdlib>
#include <assert.h>
#include <map>
#include <unordered_map>
#include <random>
#include <iostream>
#include <sys/stat.h>

#define DEBUG 1

#if DEBUG
#  define IBLT_DEBUG(x)  std::cout << x;
#else
#  define IBLT_DEBUG(x)  do {} while (0)
#endif


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

/**  checkResults asserts that two sets contain the same information**/
template <typename key_type>
void checkResults(std::unordered_map<key_type, std::vector<int> >& expected, 
				  std::unordered_map<key_type, std::vector<int> >& actual) {
	if( expected.size() != actual.size() ) {
		printf("Expected: %zu, Actual: %zu\n", expected.size(), actual.size());
		return;
	}

	for(auto it1 = expected.begin(); it1 != expected.end(); ++it1 ) {
		assert( actual[it1->first] == it1->second );
	}
}

size_t load_buffer_with_file(const char* filename, char** buf);

// generate_random_file creates a file with len alphanumeric characters
void generate_random_file(const char* filename, size_t len);

// generate_similar_file creates a file that has on average pct_similarity characters the same 
// and in the same order as the old_file
void generate_similar_file(const char* old_file, const char* new_file, double pct_similarity);

template <typename key_type, int key_bits = 8*sizeof(key_type)>
class keyGenerator {
  public:
  	std::default_random_engine rng;
	std::uniform_int_distribution<uint64_t> dist;

	keyGenerator(): dist(0, (uint64_t) -1) {}
	key_type generate_key() {
		return (key_type) dist(rng);
	}

	void set_seed(int s) {
		rng.seed(s);
	}
};

template <int key_bits>
class keyGenerator<std::string, key_bits> {
  public:
  	int key_bytes;
  	static const std::string alphanumeric;
        
    std::default_random_engine rng;
	std::uniform_int_distribution<> dist;

  	keyGenerator(): key_bytes(key_bits/8), dist(0, alphanumeric.size() - 1) {}
  	void set_seed(int s) {
  		rng.seed(s);
  	}

  	std::string generate_key() {
  		std::string output;
  		output.reserve(key_bytes);
  		for(int i = 0; i < key_bytes; ++i) {
  			output += alphanumeric[dist(rng)];
  		}
  		return output;
  	}
};

template <int key_bits>
const std::string keyGenerator<std::string, key_bits>::alphanumeric = 
		"abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789 ";

template <typename key_type, typename generator = keyGenerator<key_type, sizeof(key_type)> >
class keyHandler {
  public:
  	generator gen;
	void generate_distinct_keys(int num_keys, std::unordered_set<key_type>& keys) {
		int num_inserted = 0;
		while( num_inserted != num_keys) {
			if( keys.insert( gen.generate_key() ).second ) {
				//printf("Inserting key %ld\n", new_key);
				++num_inserted;
			}
		}
	}

	void insert_sample_keys(int num_shared_keys, int num_distinct_keys, std::unordered_set<key_type> all_keys,
							std::unordered_set<key_type>& shared_keys, std::vector<std::unordered_set<key_type> >& key_sets) {
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

	void generate_sample_keys(int num_shared_keys, int num_distinct_keys,
								std::unordered_set<key_type>& shared_keys,
								std::vector<std::unordered_set<key_type> >& key_sets) {
		std::unordered_set<key_type> all_keys;
		generate_distinct_keys(num_shared_keys + key_sets.size()*num_distinct_keys, all_keys);
		insert_sample_keys(num_shared_keys, num_distinct_keys, all_keys, shared_keys, key_sets);
	}

	void assign_keys(double insert_prob, int n_parties, std::unordered_set<key_type>& all_keys, 
					 std::unordered_map<key_type, std::vector<int> >& key_assignments) {
		std::random_device rd;
	    std::mt19937 gen(rd());
	    std::uniform_real_distribution<> dis(0, 1);
		for(auto it = all_keys.begin(); it != all_keys.end(); ++it) {
			std::vector<int> assignments;
			for(int i = 0; i < n_parties; ++i) {
				if( dis(gen) < insert_prob) {
					assignments.push_back(i);
				}
			}
			key_assignments[*it] = assignments;
		}
	}

	void assign_keys(double insert_prob, int n_parties, int num_keys, 
					 std::unordered_map<key_type, std::vector<int> >& key_assignments) {
		std::unordered_set<key_type> all_keys;
		generate_distinct_keys(num_keys, all_keys);
		assign_keys( insert_prob, n_parties, all_keys, key_assignments);
	}

	void set_union(std::vector<std::unordered_set<key_type> >& key_sets, std::unordered_set<key_type>& final_set) {
		for(auto it1 = key_sets.begin(); it1 != key_sets.end(); ++it1) {
			for(auto it2 = (*it1).begin(); it2 != (*it1).end(); ++it2) {
				final_set.insert(*it2);
			}
		}
	}

	void set_union(std::unordered_set<key_type>& key1, std::unordered_set<key_type>& key2, std::unordered_set<key_type>& final_set) {
		for(auto it = key1.begin(); it != key1.end(); ++it) {
			final_set.insert(*it);
		}
		for(auto it = key2.begin(); it != key2.end(); ++it) {
			final_set.insert(*it);
		}
	}
	void set_intersection(std::unordered_set<key_type>& key1, std::unordered_set<key_type>& key2, std::unordered_set<key_type>& intersection) {
		for(auto it1 = key1.begin(); it1 != key1.end(); ++it1) {
			if( key2.find(*it1) != key2.end() ) {
				intersection.insert(*it1);
			}
		}
	}

	void set_counts(std::vector<std::unordered_set<key_type> >& key_sets,
					std::unordered_map<key_type, int>& counts) {
		for(auto it1 = key_sets.begin(); it1 != key_sets.end(); ++it1) {
			for(auto it2 = (*it1).begin(); it2 != (*it1).end(); ++it2) {
				if( counts.find(*it1) == counts.end() ) {
					counts[*it1] = 1;
				} else {
					counts[*it1]++;
				}
			}
		}
	}

	void set_counts(std::unordered_map<key_type, std::vector<int> >& key_assignments,
					std::unordered_map<key_type, int>& counts) {
		for(auto it = key_assignments.begin(); it != key_assignments.end(); ++it) {
			counts[it->first] = (it->second).size();
		}
	}

	void set_difference(size_t n_parties, std::unordered_map<key_type, std::vector<int> >& key_assignments,
						std::unordered_set<key_type>& keys) {
		for(auto it = key_assignments.begin(); it != key_assignments.end(); ++it) {
			if((it->second.size()) > 0 && (it->second.size() < n_parties))
				keys.insert(it->first);
		}
	}

	void set_difference(size_t n_parties, std::unordered_map<key_type, std::vector<int> >& key_assignments,
						std::unordered_map<key_type, std::vector<int> >& keys) {
		for(auto it = key_assignments.begin(); it != key_assignments.end(); ++it) {
			if((it->second.size()) > 0 && (it->second.size() < n_parties)) {
				keys[it->first] = it->second;
			} 
		}
	}	
};

#endif