#ifndef _NETWORK
#define _NETWORK

#include <vector>
#include "IBLT_helpers.hpp"
#include "basicField.hpp"
#include "multiIBLT.hpp"

#define SMALL_PRIME 251

#define NETWORK_DEBUG 1
#if NETWORK_DEBUG
#  define NET_DEBUG(x)  do { std::cerr << x << std::endl; } while(0)
#else
#  define NET_DEBUG(x)  do {} while (0)
#endif


template <int n_nodes>
class iblt_node {
    public:
	const int num_hfs = 4;
	typedef uint32_t key_type;
	typedef uint32_t hash_type;
	static const int key_bits = 8*sizeof(key_type);
	typedef multiIBLT_bucket<SMALL_PRIME, key_type, key_bits, hash_type> bucket_type;
	typedef multiIBLT<n_nodes, key_type, key_bits, hash_type, bucket_type> iblt_type; 
	iblt_type* start_iblt;
	iblt_type* inter_iblt;
	SimpleField<SMALL_PRIME> coeff_sum;
	keyGenerator<uint32_t> kg;
 	int id;	
	iblt_node( int num_buckets, int seed ) {
		id = seed;
		kg.set_seed(id);
		start_iblt = new iblt_type(num_buckets, num_hfs);
		inter_iblt = new iblt_type(num_buckets, num_hfs);
	}	
	
	~iblt_node() {
		delete start_iblt;
		delete inter_iblt;
	}

	void setup(std::unordered_set<key_type>& messages) {
		NET_DEBUG("Node " << id << " has " << messages.size() << " messages");
		start_iblt->insert_keys(messages);
		inter_iblt->insert_keys(messages);
		coeff_sum.add(1);
	}

	void receive_message( iblt_node<n_nodes>& neighbor ) {
		inter_iblt->add(*(neighbor.inter_iblt));
		coeff_sum.add(neighbor.coeff_sum);
	}

	void send_message( iblt_node<n_nodes>& neighbor ) {	
		int rand_mult = (kg.generate_key() % (SMALL_PRIME - 1)) + 1;
		inter_iblt->multiply(rand_mult);
		coeff_sum.multiply(rand_mult);
		neighbor.receive_message(*this);
	}

	bool retrieve_messages(std::unordered_set<key_type>& messages) {
		iblt_type inter_iblt_tmp = iblt_type(*inter_iblt);
		iblt_type start_iblt_tmp = iblt_type(*start_iblt);
		int mult_factor = SMALL_PRIME - coeff_sum.arg;
		start_iblt_tmp.multiply( mult_factor );
		inter_iblt_tmp.add(start_iblt_tmp);
		return inter_iblt_tmp.peel(messages);
	}
};

template <int n_nodes, class node_type = iblt_node<n_nodes> > 
class GossipNetwork {
    public:
	typedef typename node_type::key_type key_type;
	enum network_type { COMPLETE, RANDOM };
	std::vector<std::vector<int>> adjacencies;
	std::vector<node_type*> nodes;
	keyGenerator<uint32_t> kg;
	
	GossipNetwork(network_type type, int set_diff_bound): adjacencies(n_nodes), nodes(n_nodes) {
		if(type == COMPLETE) {
			for(int i = 0; i < n_nodes; ++i) {
				for(int j = 0; j < n_nodes; ++j) {
					if( i != j )
						adjacencies[i].push_back(j);
				}
			}
		}
		for(int i = 0; i < n_nodes; ++i ) {
			nodes[i] = new node_type(set_diff_bound, i); 
		}
		
		kg.set_seed(0);
	}

	~GossipNetwork() {
		for(int i = 0; i < n_nodes; ++i ) 
			delete nodes[i];
	}

	void setup(std::vector<std::unordered_set<key_type> >& key_assignments) {
		for(int i = 0; i < n_nodes; ++i) {
			nodes[i]->setup(key_assignments[i]);
		}
	}

	void run_iter() {
		for(int i = 0; i < n_nodes; ++i) {
			push(i);
		}
	}
	
	void push(int i) {
		int j = kg.generate_key() % (adjacencies[i].size() + 1);
		nodes[i]->send_message(*nodes[j]);
		NET_DEBUG( i << " sending message to " << j );
	}

	void peel_keys(std::vector<std::unordered_set<key_type> >& keys ) {
		for(int i = 0; i < n_nodes; ++i) {
			bool res = nodes[i]->retrieve_messages(keys[i]);
			NET_DEBUG("Result of peeling for " << i << " is " << res);
		}
	}

	
};
#endif

