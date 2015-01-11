#include <vector>
#include "multiIBLT.hpp"
#include "MurmurHash2.hpp"

template <size_t n_parties, typename key_type, size_t key_bits = 8*sizeof(key_type)>
class StrataEstimator {
  public:
  	typedef multiIBLT<n_parties, key_type, key_bits> iblt_type;
  	const int num_buckets = 80;
  	const int num_hfs = 4;
  	std::vector<iblt_type*> iblts;

  	StrataEstimator() {
  		iblts.resize(key_bits);
  		for(size_t i = 0; i < iblts.size(); ++i) {
  			iblts[i] = new iblt_type(num_buckets, num_hfs);
  		}
  	}

  	void add(StrataEstimator<n_parties, key_type, key_bits>& cp, int index) {
		for(int i = key_bits - 1; i >= 0; --i) {
  			iblts[i]->add(*(cp.iblts[i]), index);
		}
  	}

  	size_t set_diff_estimate() {
  		size_t count = 0;
  		for(int i = key_bits - 1; i >= 0; --i) {
  			std::unordered_map<key_type, std::vector<int> > peeled_keys;
  			bool res = iblts[i]->peel(peeled_keys);
  			if( !res ) {
  				return (1 << (i+1)) * count;
  			}
  			count += peeled_keys.size();
  		}
  		return count;
  	}
  	
  	// size_t set_diff_estimate(StrataEstimator<n_parties, key_type, key_bits>& cp) {
  	// 	size_t count = 0;
  	// 	for(int i = key_bits - 1; i >= 0; --i) {
  	// 		std::unordered_map<key_type, std::vector<int> > peeled_keys;
  	// 		iblts[i]->add(*(cp.iblts[i]), 1);
  	// 		bool res = iblts[i]->peel(peeled_keys);
  	// 		if( !res ) {
  	// 			return (1 << (i+1)) * count;
  	// 		}
  	// 		count += peeled_keys.size();
  	// 	}
  	// 	return count;
  	// }

  	void insert_key(key_type k) {
  		key_type hash = MurmurHash64A ( &k, key_bits/8, 0 );
  		iblts[num_trailing_zeroes(hash)]->insert_key(k);
  	}

  	int num_trailing_zeroes(key_type k) {
  		key_type curr_mask = 1;
  		for(size_t i = 0; i < key_bits; ++i) {
  			if( curr_mask & k ) {
  				return i;
  			}
  			curr_mask = (curr_mask << 1) + 1;
  		}
  		return key_bits;
  	}

};