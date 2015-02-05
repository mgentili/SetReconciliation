#ifndef _NETWORK
#define _NETWORK

#include <vector>
#include <cmath>

#include "IBLT_helpers.hpp"
#include "basicField.hpp"
#include "multiIBLT.hpp"

//#define PRIME 17
//#define PRIME 101
//#define PRIME 251
//#define PRIME 65027
//#define PRIME 14653
//#define PRIME 39373
//#define PRIME 104729
#define PRIME 860117
#define NETWORK_DEBUG 0
#if NETWORK_DEBUG
#  define NET_DEBUG(x)  do { std::cerr << x << std::endl; } while(0)
#else
#  define NET_DEBUG(x)  do {} while (0)
#endif

class network_type {
    public:
	int n_nodes;
	network_type(int num_nodes ): n_nodes(num_nodes) {}

	virtual void create_network(std::vector<std::vector<int> >& list) = 0;
};

class complete_network : public network_type {
    public:
	complete_network(int n_nodes): network_type(n_nodes) {}

	void create_network(std::vector<std::vector<int> >& list) {
		list.resize(n_nodes);
		for(int i = 0; i < n_nodes; ++i ) {
			for(int j = 0; j < n_nodes; ++j) {
				if( i != j) 
					list[i].push_back(j);
			}
		}
	}
};

class random_network: public network_type {
    public:
	double p;

	std::random_device rd;
	std::mt19937 gen;
	std::uniform_real_distribution<> dis;	
		
	random_network(int n_nodes, double edge_prob): network_type(n_nodes), p(edge_prob), gen(rd()), dis(0,1) {}

	random_network(int n_nodes): network_type(n_nodes), gen(rd()), dis(0,1) {
		p = 2*log(n_nodes)/n_nodes;
		NET_DEBUG( "Probability of edge existing is " << p );	
	};

	void create_network(std::vector<std::vector<int> >& list) {
		list.resize(n_nodes);
		for(int i = 0; i < n_nodes; ++i ) {
			for(int j = 0; j < i; ++j) {
				if( dis(gen) < p ) {
					list[i].push_back(j);
					list[j].push_back(i);	
				}
			}
		}
	}
};

template <int n_nodes, typename key_type = uint64_t, typename hash_type = uint64_t>
class iblt_node {
    public:
	const int num_hfs = 4;
	static const int key_bits = 8*sizeof(key_type);
	
	typedef multiIBLT_bucket<PRIME, key_type, key_bits, hash_type> bucket_type;
	typedef multiIBLT<n_nodes, key_type, key_bits, hash_type, bucket_type> iblt_type; 
	typedef iblt_node<n_nodes, key_type, hash_type> node_type; 
	iblt_type* start_iblt; //IBLT containing all the keys that the node begins with
	iblt_type* inter_iblt; //IBLT containing linear combo of keys received
	SimpleField<PRIME> coeff_sum; //sum of coefficients of linear combo of keys
	keyGenerator<key_type> kg;
 	int id; //unique identifier for node
	bool has_message;
	std::vector< node_type* > neighbors, prev_want_info, new_want_info; //nodes that have made pull requests
	std::bitset<n_nodes> received_from; //for testing purposes, says whether have received message in some form (linear combo)	
	
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
		has_message = (messages.size() > 0 );
		received_from.set(id);
		NET_DEBUG( id << " has received " << received_from.count() << " messages");
	}
	
	void new_epoch() {
		prev_want_info = new_want_info;
		new_want_info.resize(0);
	}
	
	bool all_messages_received() {
		return received_from.all();
	}

	void push() {
		int j = kg.generate_key() % neighbors.size();
		send_message(*neighbors[j]);

	}

	void pull() {
		int j = kg.generate_key() % neighbors.size();
		send_pull_request(*neighbors[j]);
	}

	void push_pull() {
		if( has_message ) {
			size_t wanters = prev_want_info.size();
			if( wanters > 0) { //if someone wants message, then give it to them
				int j = kg.generate_key() % wanters;
				send_message(*prev_want_info[j]);
			} else {
				push();
			}
		} else {
			pull();
		}
	}

	void send_message( iblt_node<n_nodes>& neighbor ) {	
		int rand_mult = (kg.generate_key() % (PRIME - 1)) + 1;
		inter_iblt->multiply(rand_mult);
		coeff_sum.multiply(rand_mult);
		NET_DEBUG( id << " pushing message to " << neighbor.id );
		neighbor.receive_message(*this);
	}
	
	void receive_message( iblt_node<n_nodes>& neighbor ) {
		inter_iblt->add(*(neighbor.inter_iblt));
		coeff_sum.add(neighbor.coeff_sum);
		has_message = true;
		received_from |= neighbor.received_from;
		NET_DEBUG( id << " has received " << received_from.count() << " messages");
	}

	void send_pull_request( iblt_node<n_nodes>& neighbor ) {
		NET_DEBUG( id << " sending pull request to " << neighbor.id );
		neighbor.receive_pull_request(*this);
	}

	void receive_pull_request( iblt_node<n_nodes>& neighbor ) {
		new_want_info.push_back( &neighbor );
	}

	bool retrieve_messages(std::unordered_set<key_type>& messages) {
		iblt_type inter_iblt_tmp = iblt_type(*inter_iblt);
		iblt_type start_iblt_tmp = iblt_type(*start_iblt);
		int mult_factor = PRIME - coeff_sum.arg;
		start_iblt_tmp.multiply( mult_factor );
		inter_iblt_tmp.add(start_iblt_tmp);
		bool res = inter_iblt_tmp.peel(messages);
		if( !res ) {
			NET_DEBUG( id << " failed to peel" );
			inter_iblt_tmp.print_contents();
		}
		return res;
	}
};

template <int n_nodes, 
	  class graph_type = complete_network, 
	  typename key_type = uint64_t, 
	  typename hash_type = uint64_t, 
	  class node_type = iblt_node<n_nodes, key_type, hash_type> > 
class GossipNetwork {
    public:
	std::vector<std::vector<int>> adjacencies;
	std::vector<node_type*> nodes;
	keyGenerator<key_type> kg;
	graph_type gt;
	int iter; //iteration number
	
	GossipNetwork(int set_diff_bound): 
		adjacencies(n_nodes), 
		nodes(n_nodes), 
		gt(n_nodes),
		iter(0) {
		gt.create_network(adjacencies);
		
		for(int i = 0; i < n_nodes; ++i ) {
			nodes[i] = new node_type(set_diff_bound, i); 
		}
		
		//setup each node to contain pointers to each neighbor
		for(int i = 0; i < n_nodes; ++i) {
			for(size_t j = 0; j < adjacencies[i].size(); ++j) {
				nodes[i]->neighbors.push_back(nodes[adjacencies[i][j]]);
			}
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
			//push(i);
			nodes[i]->new_epoch();
			nodes[i]->push_pull();
		}
		++iter;
	}

	bool all_messages_received() {
		for(int i = 0; i < n_nodes; ++i) {
			if( !nodes[i]->all_messages_received() ) {
				return false;
			}
		}
		return true;
	}
	

	bool peel_keys(std::vector<std::unordered_set<key_type> >& keys ) {
		bool success = true;
		for(int i = 0; i < n_nodes; ++i) {
			bool res = nodes[i]->retrieve_messages(keys[i]);
			if( !res ) {
				NET_DEBUG("Result of peeling for " << i << " is " << res);
				success = false;
			}
		}
		return success;
	}

	
};
#endif

