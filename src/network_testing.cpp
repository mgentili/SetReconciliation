#include "network.hpp"
#include "IBLT_helpers.hpp"
#include <iostream>
#include "json/json.h"

Json::Value info;
Json::StyledWriter writer;

template <typename key_type>
void printPeeledKeys(std::vector<std::unordered_set<key_type>>& peeled_keys) {
	for(size_t i = 0; i < peeled_keys.size(); ++i) {
		std::cout << i << " has " << peeled_keys[i].size() << " keys " << std::endl;
//		for(auto it = peeled_keys[i].begin(); it != peeled_keys[i].end(); ++it) {
//			std::cout << *it << std::endl;
//		}
	}
}

template <typename key_type>
double percentSuccessfullyPeeled(std::vector<std::unordered_set<key_type>>& peeled_keys, 
			 	 std::unordered_set<key_type>& distinct_keys) {
	int denom = distinct_keys.size() * peeled_keys.size();
	int numer = 0;
	int failed_parties = 0;
	int failed_keys = 0;
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
	Json::Value num_failed_keys(denom-numer), num_failed_parties(failed_parties), pct_transmitted((double) numer/denom);
	info["failed_keys"] = num_failed_keys;
	info["num_failed_parties"] = num_failed_parties;
	info["pct_transmitted"] = pct_transmitted;

	return( numer/denom );
}

template <typename key_type>
void processPeeledKeys(std::vector<std::unordered_set<key_type> >& peeled_keys, size_t num_distinct_keys ) {
	Json::Value failed_array;
	for(auto it = peeled_keys.begin(); it != peeled_keys.end(); ++it ) {
		Json::Value new_val((int) (num_distinct_keys - it->size()));
		failed_array.append(new_val);
	}

	info["nodes"] = failed_array;
}
template <int n_nodes>
void testCompleteNetwork2() {
	typedef uint64_t key_type;
	typedef uint64_t hash_type;
	typedef GossipNetwork<n_nodes, random_network, key_type, hash_type> net_type;
	net_type net(2*n_nodes);
	keyHandler<key_type> kh;

	std::unordered_set<key_type> keys;
	std::vector<std::unordered_set<key_type> > key_assignments(n_nodes), init_peeled_keys(n_nodes);
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
	
	
	Json::Value num_distinct_keys((int) distinct_keys.size()), num_rounds((int) net.iter);
	Json::Value prime_val(net.nodes[0]->get_prime());
	info["num_distinct_keys"] = num_distinct_keys;
	info["num_rounds"] = num_rounds;
	info["prime"] = prime_val;
	processPeeledKeys<key_type>(peeled_keys, distinct_keys.size());
	percentSuccessfullyPeeled<key_type>(peeled_keys,distinct_keys);
	std::cout << writer.write( info ) << std::endl;
	info = Json::Value::null;
}

int main() {
	int num_trials = 10;
	for(int i = 0; i < num_trials; ++i) {
		testCompleteNetwork2<10>();
		testCompleteNetwork2<20>();
		testCompleteNetwork2<40>();
		testCompleteNetwork2<80>();
		testCompleteNetwork2<160>();
		testCompleteNetwork2<320>();
		testCompleteNetwork2<640>();
		testCompleteNetwork2<1280>();
		testCompleteNetwork2<2560>();
	}
	return 1;
}
