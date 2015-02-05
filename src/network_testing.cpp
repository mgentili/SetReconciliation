#include "network.hpp"
#include "IBLT_helpers.hpp"
#include <iostream>

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
	std::cout << ",failed_keys=" << denom-numer << 
		     ",failed_parties=" << failed_parties << 
		     ",pct_transmitted=" << (double) numer/denom << 
	std::endl;

	return( numer/denom );
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
	//printPeeledKeys<key_type>(key_assignments);
	std::cout << "distinct_keys=" << distinct_keys.size();
	net.setup(key_assignments);
	
//	std::cout << "Initial peeling: " << std::endl;
//	net.peel_keys(init_peeled_keys);
//	printPeeledKeys<key_type>(init_peeled_keys);
//

	while( !net.all_messages_received() ) {
		net.run_iter();
	}

	std::vector<std::unordered_set<key_type>> peeled_keys(n_nodes);
	if( !net.peel_keys(peeled_keys) ) {
		std::cout << "Failed to peel keys for all nodes" << std::endl;
	}
	//printPeeledKeys<key_type>(peeled_keys);
	std::cout << ",num_rounds=" << net.iter;
	percentSuccessfullyPeeled<key_type>(peeled_keys, distinct_keys);


}
//
//void testCompleteNetwork() {
//	const int n_nodes = 100;
//	const int set_diff = 50;
//	typedef uint32_t key_type;
//	typedef GossipNetwork<n_nodes, random_network> net_type;
//	
//	net_type net(set_diff);	
//	keyHandler<key_type> kh; 
//	std::vector<std::unordered_set<key_type>> key_assignments(n_nodes), init_peeled_keys(n_nodes);
//	double insert_prob = 0.1;
//	kh.assign_keys(insert_prob, n_nodes, set_diff/2, key_assignments);
//	std::unordered_set<key_type> distinct_keys;
//	kh.distinct_keys(key_assignments, distinct_keys);
//	//printPeeledKeys<key_type>(key_assignments);
//	std::cout << "Total distict keys: " << distinct_keys.size() << std::endl;
//	net.setup(key_assignments);
//	std::cout << "Finished setting up keys" << std::endl;
//	
//	std::cout << "Initial peeling: " << std::endl;
//	net.peel_keys(init_peeled_keys);
//	printPeeledKeys<key_type>(init_peeled_keys);
//
//
//	while( !net.all_messages_received() ) {
//		net.run_iter();
//	}
//
//	std::vector<std::unordered_set<key_type>> peeled_keys(n_nodes);
//	net.peel_keys(peeled_keys);
//	printPeeledKeys<key_type>(peeled_keys);
//	std::cout << "All messages transmitted in " << net.iter << " rounds" << std::endl;
//	std::cout << "Percent of messages transmitted: " << percentSuccessfullyPeeled(peeled_keys, distinct_keys) << std::endl;
//
//}

int main() {

	testCompleteNetwork2<10>();
	testCompleteNetwork2<20>();
	testCompleteNetwork2<40>();
	testCompleteNetwork2<80>();
	testCompleteNetwork2<160>();
	testCompleteNetwork2<320>();
	testCompleteNetwork2<640>();
	testCompleteNetwork2<1280>();
	testCompleteNetwork2<2560>();
	return 1;
}
