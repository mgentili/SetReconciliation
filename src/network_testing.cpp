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

void testCompleteNetwork() {
	const int n_nodes = 100;
	const int set_diff = 50;
	typedef uint32_t key_type;
	typedef GossipNetwork<n_nodes> net_type;
	GossipNetwork<n_nodes> net(net_type::network_type::COMPLETE, set_diff);	
	keyHandler<key_type> kh; 
	std::vector<std::unordered_set<key_type>> key_assignments(n_nodes);
	double insert_prob = 0.01;
	kh.assign_keys(insert_prob, n_nodes, set_diff, key_assignments);
	std::unordered_set<key_type> distinct_keys;
	kh.distinct_keys(key_assignments, distinct_keys);
	printPeeledKeys<key_type>(key_assignments);
	std::cout << "Total distict keys: " << distinct_keys.size() << std::endl;
	net.setup(key_assignments);
	std::cout << "Setup keys" << std::endl;
	int num_iters = n_nodes;
	for(int i = 0; i < num_iters; ++i) {
		std::vector<std::unordered_set<key_type>> peeled_keys(n_nodes);
		net.run_iter();
		net.peel_keys(peeled_keys);
		printPeeledKeys<key_type>(peeled_keys);
	}
	
	//net.peel_keys(peeled_keys);
}

int main() {
	testCompleteNetwork();
	return 1;
}
