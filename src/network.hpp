#ifndef _NETWORK
#define _NETWORK

#include <cmath>
#include <deque>
#include <vector>

#include "basicField.hpp"
#include "IBLT_helpers.hpp"
#include "multiIBLT.hpp"

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

	random_network(int n_nodes): random_network(n_nodes, 2*log(n_nodes)/n_nodes) {};
    //network_type(n_nodes), gen(rd()), dis(0,1) {
	//	p = 2*log(n_nodes)/n_nodes;
	//	NET_DEBUG( "Probability of edge existing is " << p );	
	//};

	void create_network(std::vector<std::vector<int> >& list) {
		do {
			list.clear();
			list.resize(n_nodes);
			for(int i = 0; i < n_nodes; ++i ) {
				for(int j = 0; j < i; ++j) {
					if( dis(gen) < p ) {
						list[i].push_back(j);
						list[j].push_back(i);	
					}
				}
			}
	
		} while( !is_connected(list) );
	}
	
	bool is_connected(std::vector<std::vector<int> >& list) {
		std::deque<int> dq;
		int n_nodes = list.size();
		int num_traversed = 0;		
		std::vector<bool> have_traversed(n_nodes);
		dq.push_back( 0 );
		while( !dq.empty() && (num_traversed < n_nodes) ) {
			int curr_node = dq.front();
			dq.pop_front();
			if( !have_traversed[curr_node] ) {
				++num_traversed;
			}
			have_traversed[curr_node] = true;
			if( list[curr_node].size() == 0 ) {
				return false;
			}
			for( auto it = list[curr_node].begin(); it != list[curr_node].end(); ++it) {
				if( !have_traversed[*it] ) {
					dq.push_back(*it);
				}
			}
		} 
		return (num_traversed == n_nodes);	
		
	}
};

template <int n_nodes, int prime, typename key_type = uint32_t, typename hash_type = uint32_t>
class iblt_node {
    public:
	const int num_hfs = 4;
	static const int key_bits = 8*sizeof(key_type);
	
	typedef multiIBLT_bucket<prime, key_type, key_bits, hash_type> bucket_type;
	typedef multiIBLT<n_nodes, key_type, key_bits, hash_type, bucket_type> iblt_type; 
	typedef iblt_node<n_nodes, prime, key_type, hash_type> node_type; 
	iblt_type* start_iblt; //IBLT containing all the keys that the node begins with
	iblt_type* inter_iblt; //IBLT containing linear combo of keys received
	iblt_type* temp_iblt; //IBLT containing linear combo received during one round
	SimpleField<prime> coeff_sum, temp_coeff_sum; //sum of coefficients of linear combo of keys
	keyGenerator<key_type> kg;
 	int id; //unique identifier for node
	std::vector< node_type* > neighbors;
	std::bitset<n_nodes> received_from, temp_received_from; //for testing purposes, says whether have received message in some form (linear combo)	
	iblt_node( int num_buckets, int seed ) {
		id = seed;
		kg.set_seed(id);
		start_iblt = new iblt_type(num_buckets, num_hfs);
		inter_iblt = new iblt_type(num_buckets, num_hfs);
		temp_iblt  = new iblt_type(num_buckets, num_hfs);
	}	
	
	~iblt_node() {
		delete start_iblt;
		delete inter_iblt;
		delete temp_iblt;
	}
	int get_prime() {
		return prime;
	}
	void setup(std::unordered_set<key_type>& messages) {
		NET_DEBUG("Node " << id << " has " << messages.size() << " messages");
		start_iblt->insert_keys(messages);
		inter_iblt->insert_keys(messages);
		coeff_sum.add(1);
		received_from.set(id);
		NET_DEBUG( id << " has received " << received_from.count() << " messages");
	}
	
	void end_epoch() {
		inter_iblt->add(*temp_iblt);
		coeff_sum.add(temp_coeff_sum); 	
		delete temp_iblt;
		temp_iblt = new iblt_type(start_iblt->num_buckets, start_iblt->num_hashfns);
		temp_coeff_sum.set(0);
		received_from |= temp_received_from;
		temp_received_from.reset();
	}
	
	bool all_messages_received() {
		return received_from.all();
	}

	void push_pull() {
		int j = kg.generate_key() % neighbors.size();
		send_message(*neighbors[j]);
		receive_message(*neighbors[j]);	
	}

	//TODO: Temp iblt before multiplying?
	void send_message( node_type& neighbor ) {	
		int rand_mult = (kg.generate_key() % (prime - 1)) + 1;
		iblt_type message= iblt_type(*inter_iblt);
		SimpleField<prime> message_sum(coeff_sum); 
		message.multiply(rand_mult);
		message_sum.multiply(rand_mult);
		NET_DEBUG( id << " pushing message to " << neighbor.id );
		neighbor.receive_message(*this, message, message_sum);
	}
	
	void receive_message( node_type& neighbor, iblt_type& message, SimpleField<prime>& message_sum ) {
		temp_iblt->add(message);
		temp_coeff_sum.add(message_sum);
		temp_received_from |= neighbor.received_from;
		NET_DEBUG( id << " has received " << received_from.count() << " messages");
	}
	
	void receive_message( node_type& neighbor ) {
		temp_iblt->add(*(neighbor.inter_iblt));
		temp_coeff_sum.add(neighbor.coeff_sum);
		temp_received_from |= neighbor.received_from;
	}

	bool retrieve_messages(std::unordered_set<key_type>& messages) {
		iblt_type inter_iblt_tmp = iblt_type(*inter_iblt);
		iblt_type start_iblt_tmp = iblt_type(*start_iblt);
		int mult_factor = prime - coeff_sum.arg;
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
	  int prime,
	  class graph_type = complete_network, 
	  typename key_type = uint32_t, 
	  typename hash_type = uint32_t, 
	  class node_type = iblt_node<n_nodes, prime, key_type, hash_type> > 
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
			nodes[i]->push_pull();
			nodes[i]->end_epoch();
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

