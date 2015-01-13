#include <vector>
#include "multiIBLT.hpp"
#include "hash_util.hpp"
#include "file_sync.pb.h"

template <size_t n_parties, typename hash_type = uint16_t>
class StrataEstimator {
  public:
  	const int num_buckets = 40;
  	const int num_hfs = 4;
    static const size_t num_strata = 8*sizeof(hash_type);
    typedef multiIBLT<n_parties, hash_type, num_strata> iblt_type;
  	std::vector<iblt_type*> iblts;

  	StrataEstimator(): iblts(num_strata) {
  		for(size_t i = 0; i < iblts.size(); ++i) {
  			iblts[i] = new iblt_type(num_buckets, num_hfs);
  		}
  	}

    void serialize(file_sync::strata_estimator& estimator) {
      for(size_t i = 0; i < num_strata; ++i) {
        file_sync::IBLT* new_iblt = estimator.add_strata();
        iblts[i]->serialize(*new_iblt);
      }
    }

    void deserialize(const file_sync::strata_estimator& estimator) {
      for(size_t i = 0; i < num_strata; ++i) {
        iblts[i]->deserialize(estimator.strata(i));
      }
    }

  	void add(StrataEstimator<n_parties, hash_type>& cp, int index) {
		for(int i = num_strata - 1; i >= 0; --i) {
  			iblts[i]->add(*(cp.iblts[i]), index);
		}
  	}

  	size_t set_diff_estimate() {
  		size_t count = 0;
  		for(int i = num_strata - 1; i >= 0; --i) {
  			std::unordered_map<hash_type, std::vector<int> > peeled_keys;
  			bool res = iblts[i]->peel(peeled_keys);
  			if( !res ) {
  				return (1 << (i+1)) * count;
  			}
  			count += peeled_keys.size();
  		}
  		return count;
  	}
    void insert_key(const std::string& s) {
      hash_type hash = HashUtil::MurmurHash64A( s.c_str(), s.size(), 0 );
      iblts[num_trailing_zeroes(hash)]->insert_key(hash);
    }

  	void insert_key(const hash_type k) {
  		hash_type hash = HashUtil::MurmurHash64A ( &k, num_strata/8, 0 );
  		iblts[num_trailing_zeroes(hash)]->insert_key(hash);
  	}

  	int num_trailing_zeroes(hash_type k) {
  		hash_type curr_mask = 1;
  		for(size_t i = 0; i < num_strata; ++i) {
  			if( curr_mask & k ) {
  				return i;
  			}
  			curr_mask = (curr_mask << 1) + 1;
  		}
  		return num_strata;
  	}

};