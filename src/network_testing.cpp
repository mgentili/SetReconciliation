#include "network.hpp"

#include <iostream>

#include "IBLT_helpers.hpp"
#include "json/json.h"

//#define PRIME 17
//#define PRIME 101
//#define PRIME 251
//#define PRIME 65027
#define SMALL_PRIME 14653
//#define PRIME 39373
//#define PRIME 104729
#define BIG_PRIME 860117
#define MASSIVE_PRIME 67867979
#define GINORMO_PRIME 1000000007

Json::Value info;
Json::StyledWriter writer;

template <typename key_type>
void printPeeledKeys(std::vector<std::unordered_set<key_type>>& peeled_keys) {
	for(size_t i = 0; i < peeled_keys.size(); ++i) {
		std::cout << i << " has " << peeled_keys[i].size() << " keys " << std::endl;
	}
}

template <typename key_type>
double percentSuccessfullyPeeled(std::vector<std::unordered_set<key_type>>& peeled_keys, 
			 	 std::unordered_set<key_type>& distinct_keys) {
	int denom = distinct_keys.size() * peeled_keys.size();
	int numer = 0;
	int failed_parties = 0;
	for(auto it = peeled_keys.begin(); it != peeled_keys.end(); ++it) {
		int curr_count = 0;
		for(auto it2 = (*it).begin(); it2 != (*it).end(); ++it2) {
			curr_count += ( distinct_keys.find( *it2 ) != distinct_keys.end() );
		}
		if( curr_count != distinct_keys.size() ) {
			++failed_parties;
		}
		numer += curr_count;
	}
	Json::Value num_failed_keys(denom-numer), 
                num_failed_parties(failed_parties), 
                pct_transmitted((double) numer/denom);
	info["failed_keys"] = num_failed_keys;
	info["num_failed_parties"] = num_failed_parties;
	info["pct_transmitted"] = pct_transmitted;
	info["num_failed_keys"] = num_failed_keys;
	return( numer/denom );
}

template <typename key_type>
void processPeeledKeys(std::vector<std::unordered_set<key_type> >& peeled_keys, 
                       size_t num_distinct_keys ) {
	Json::Value failed_array;
	for(auto it = peeled_keys.begin(); it != peeled_keys.end(); ++it ) {
		Json::Value new_val((int) (num_distinct_keys - it->size()));
		failed_array.append(new_val);
	}

	info["nodes"] = failed_array;
}
template <int n_nodes, int prime>
void testCompleteNetwork2() {
	typedef uint64_t key_type;
	typedef uint64_t hash_type;
	typedef GossipNetwork<n_nodes, prime, random_network, key_type, hash_type> net_type;
	net_type net(2*n_nodes);
	keyHandler<key_type> kh;

	std::unordered_set<key_type> keys;
	std::vector<std::unordered_set<key_type> > key_assignments(n_nodes), 
                                               init_peeled_keys(n_nodes);
	kh.generate_distinct_keys(n_nodes, keys);
	int i = 0;
	for(auto it = keys.begin(); it != keys.end(); ++it, ++i ) {
		key_assignments[i].insert(*it);
	}
	std::unordered_set<key_type> distinct_keys;
	kh.distinct_keys(key_assignments, distinct_keys);
	net.setup(key_assignments);
	
	while( !net.all_messages_received() ) {
		net.run_iter();
	}

	std::vector<std::unordered_set<key_type>> peeled_keys(n_nodes);
	if( !net.peel_keys(peeled_keys) ) {
		std::cout << "Failed to peel keys for all nodes" << std::endl;
	}
	
	
	Json::Value num_distinct_keys((int) distinct_keys.size()), 
                num_rounds((int) net.iter),
                prime_val(net.nodes[0]->get_prime());
	info["num_distinct_keys"] = num_distinct_keys;
	info["num_rounds"] = num_rounds;
	info["prime"] = prime_val;
	processPeeledKeys<key_type>(peeled_keys, distinct_keys.size());
	percentSuccessfullyPeeled<key_type>(peeled_keys,distinct_keys);
	std::cout << writer.write( info ) << std::endl;
	info = Json::Value::null;
}

int main() {
	int num_trials = 20;
	for(int i = 0; i < num_trials; ++i) {
		testCompleteNetwork2<10, GINORMO_PRIME>();
		testCompleteNetwork2<20, GINORMO_PRIME>();
		testCompleteNetwork2<40, GINORMO_PRIME>();
		testCompleteNetwork2<80, GINORMO_PRIME>();
		testCompleteNetwork2<160, GINORMO_PRIME>();
		testCompleteNetwork2<320, GINORMO_PRIME>();
		testCompleteNetwork2<640, GINORMO_PRIME>();
		testCompleteNetwork2<1280, GINORMO_PRIME>();
	}

//	for(int i = 0; i < num_trials; ++i) {
//		testCompleteNetwork2<10, SMALL_PRIME>();
//		testCompleteNetwork2<20, SMALL_PRIME>();
//		testCompleteNetwork2<40, SMALL_PRIME>();
//		testCompleteNetwork2<80, SMALL_PRIME>();
//		testCompleteNetwork2<160, SMALL_PRIME>();
//		testCompleteNetwork2<320, SMALL_PRIME>();
//		testCompleteNetwork2<640, SMALL_PRIME>();
//		testCompleteNetwork2<1280, SMALL_PRIME>();
//	}
	return 1;
}
