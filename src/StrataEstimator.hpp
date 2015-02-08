#include <vector>
#include "basicIBLT.hpp"
#include "multiIBLT.hpp"
#include "hash_util.hpp"
#include "file_sync.pb.h"

//TODO: Paramaterize by IBLT type?
template <typename hash_type, typename iblt_type = basicIBLT<hash_type> >
//template <typename hash_type, typename iblt_type = multiIBLT<2, hash_type, 8*sizeof(hash_type), hash_type> >
class StrataEstimator {
  public:
	const int num_buckets = 80;
	const int num_hfs = 4;
	static const size_t num_strata = 8*sizeof(hash_type);
	std::vector<iblt_type*> iblts;

	StrataEstimator(): iblts(num_strata) {
		for(size_t i = 0; i < iblts.size(); ++i) {
			iblts[i] = new iblt_type(num_buckets, num_hfs);
		}
	}

	~StrataEstimator() {
		for(auto it = iblts.begin(); it != iblts.end() ; ++it)
			delete *it;
	}

	size_t size_in_bits() const {
		return num_strata*iblts[0]->size_in_bits();
	}

	void serialize(file_sync::strata_estimator& estimator) {
	  	for(size_t i = 0; i < num_strata; ++i) {
			file_sync::IBLT2* new_iblt = estimator.add_strata();
			iblts[i]->serialize(*new_iblt);
	  	}
	}

	void deserialize(const file_sync::strata_estimator& estimator) {
	  	for(size_t i = 0; i < num_strata; ++i) {
			iblts[i]->deserialize(estimator.strata(i));
	  	}
	}

	void add(StrataEstimator<hash_type, iblt_type>& cp) {
		for(int i = num_strata - 1; i >= 0; --i) {
			iblts[i]->add(*(cp.iblts[i]));
		}
	}

	void remove(StrataEstimator<hash_type, iblt_type>& cp) {
	  	for(int i = num_strata - 1; i >= 0; --i) {
		  	iblts[i]->remove(*(cp.iblts[i]));
		}
  	}

	size_t estimate_diff(StrataEstimator<hash_type, iblt_type>& cp) {
	  	remove(cp);
		size_t count = 0;
		for(int i = num_strata - 1; i >= 0; --i) {
			std::unordered_set<hash_type> peeled_keys;
			bool res = iblts[i]->peel(peeled_keys);
			// std::cout << "Peeled number of keys: " << peeled_keys.size() << std::endl;
			if( !res ) {
				// std::cout << "Failed to peel on layer " << i << std::endl;
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

	static int num_trailing_zeroes(hash_type k) {
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
