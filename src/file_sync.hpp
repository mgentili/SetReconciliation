#ifndef _FILE_SYNC
#define _FILE_SYNC

#include <map>
#include <assert.h>
#include "multiIBLT.hpp"
#include "fingerprinting.hpp"
#include "IBLT_helpers.hpp"
#include "file_sync.pb.h"
#include "StrataEstimator.hpp"

template <size_t n_parties = 2, typename hash_type = uint64_t>
class FileSynchronizer {
  public:
  	const size_t kgrams = 10;
  	const size_t window_size = 100;
  	typedef multiIBLT<n_parties, hash_type> iblt_type;

	class Round1Info {
	  public:
  	  	StrataEstimator<n_parties, hash_type> estimator;
	  	typedef multiIBLT<n_parties, hash_type> iblt_type;
	  	std::vector<pair<hash_type, size_t> > hashes; //hash and length
	  	std::map<hash_type, std::pair<size_t, size_t> > hashes_to_poslen; //mapping from hash to position in file and length
	  	iblt_type* iblt;
	};

	class Round2Info { //info to update A to have the same file as B
	  public:
		std::vector<bool> chunk_exists;  //for each chunk B has, whether A has
		std::vector<std::string> new_chunk_info; //length and contents of each chunk that A doesn't have
		std::vector<size_t> existing_chunk_encoding; //index of chunk hash within client's 

		size_t size_in_bits() { // in bits
			size_t tot_bits = 0;
			tot_bits += chunk_exists.size();
			for(auto it = new_chunk_info.begin(); it != new_chunk_info.end(); ++it) {
				tot_bits += it->size()*8;
			}
			tot_bits += existing_chunk_encoding.size() * 32;
			return tot_bits;
		}

		void serialize(file_sync::Round2& rd2) {
			for(auto it = chunk_exists.begin(); it != chunk_exists.end(); ++it) {
				rd2.add_chunk_exists(*it);
			}

			for(auto it = new_chunk_info.begin(); it != new_chunk_info.end(); ++it) {
				rd2.add_new_chunk_info(*it);
			}

			for(auto it = existing_chunk_encoding.begin(); it != existing_chunk_encoding.end(); ++it) {
				rd2.add_existing_chunk_encoding(*it);
			}
		}

		void deserialize(file_sync::Round2& rd2) {
			for(int i = 0; i < rd2.chunk_exists_size(); ++i) {
				chunk_exists.push_back(rd2.chunk_exists(i));
			}

			for(int i = 0; i < rd2.new_chunk_info_size(); ++i) {
				new_chunk_info.push_back(rd2.new_chunk_info(i));
			}

			for(int i = 0; i < rd2.existing_chunk_encoding_size(); ++i) {
				existing_chunk_encoding.push_back(rd2.existing_chunk_encoding(i));
			}
		}
	};

  	FileSynchronizer() {};

  	//Party A fills his structure with info to estimate set difference
  	//and fills the rest of the hash structure while he's at it
  	void determine_differenceA(const char* filename, Round1Info& my_rd1) {
  		Fingerprinter<hash_type> f(kgrams, window_size);
		f.digest_file( filename, my_rd1.hashes);

		// create mapping from my hashes to their block lengths and start position in file
		size_t curr_pos = 0;
  		for(auto it = my_rd1.hashes.begin(); it != my_rd1.hashes.end(); ++it) {
  			if( my_rd1.hashes_to_poslen.find(it->first) != my_rd1.hashes_to_poslen.end() ) {
  				IBLT_DEBUG("Already have hash " << it->first << " pos " << curr_pos << " len" << it->second);
  			} else {
  				my_rd1.hashes_to_poslen[it->first] = make_pair(curr_pos, it->second);
  			}
  			IBLT_DEBUG("File" << filename << " Hash " << it->first << " pos " << curr_pos << " len" << it->second);
			curr_pos += it->second;
  		}
  		IBLT_DEBUG("File " << filename << " has " << my_rd1.hashes.size() << "hashes" << ". hashes_to_pos_len has size" << my_rd1.hashes_to_poslen.size());
  	 	for(auto it = my_rd1.hashes_to_poslen.begin(); it != my_rd1.hashes_to_poslen.end(); ++it) {
  	 		my_rd1.estimator.insert_key(it->first);
  	 	}
  	}

  	size_t determine_differenceB(const char* filename, Round1Info& my_rd1, StrataEstimator<n_parties, hash_type>& cp_estimator ) {
  		determine_differenceA(filename, my_rd1);
  		my_rd1.estimator.add(cp_estimator, 1);
  		size_t diff_est = my_rd1.estimator.set_diff_estimate();
  		return diff_est;
  	}

  	void send_IBLT(Round1Info& my_rd1, size_t bucket_estimate) {
  		size_t num_buckets = bucket_estimate * 2;
  		size_t num_hashfns = ( bucket_estimate < 200 ) ? 3 : 4;
  		my_rd1.iblt = new iblt_type(num_buckets, num_hashfns);
		fill_IBLT(my_rd1);		
  	}

  	void fill_IBLT(Round1Info& my_rd1) {
  		for(auto it = my_rd1.hashes_to_poslen.begin(); it != my_rd1.hashes_to_poslen.end(); ++it) {
			my_rd1.iblt->insert_key(it->first);
		}
  	}

  	void receive_IBLT(const char* filename, Round1Info& my_rd1, iblt_type& cp_IBLT, Round2Info& my_rd2) {
  		iblt_type resIBLT(cp_IBLT.num_buckets, cp_IBLT.num_hashfns);
  		my_rd1.iblt = new iblt_type(cp_IBLT.num_buckets, cp_IBLT.num_hashfns);
		fill_IBLT(my_rd1);	

  		resIBLT.add(*(my_rd1.iblt), 0); // my unique keys are in index 0
  		resIBLT.add(cp_IBLT, 1); // counterparty's unique keys are in index 1
  		std::unordered_map<hash_type, std::vector<int> > distinct_keys;
  		resIBLT.peel(distinct_keys);

  		std::cout << "Peeled distinct keys" << distinct_keys.size() << std::endl;
  		// find keys that are unique to self and counterparty. Note, cannot use peeling directly
  		// because still haven't fixed problem with even number of parties
  		std::unordered_set<hash_type> A_unique_keys;
  		std::unordered_set<hash_type> B_unique_keys;
  		for(auto it = distinct_keys.begin(); it != distinct_keys.end(); ++it) {
  			assert( it->second.size() == 1); //only two parties for now
  			IBLT_DEBUG("Distinct key " << it->first);
  			if( my_rd1.hashes_to_poslen.find(it->first) != my_rd1.hashes_to_poslen.end() ) {
  				B_unique_keys.insert(it->first);
  			} else {
  				A_unique_keys.insert(it->first);
  			}
  		}

  		char* buf;
		load_buffer_with_file(filename, &buf);

		std::vector<hash_type> cp_sorted_hashes;
		//Create structure of Party A's sorted hashes by going through each of Party B's hashes
		//and seeing if it is in B-A. If not, then must be in A intersect B
		for(auto it = my_rd1.hashes_to_poslen.begin(); it != my_rd1.hashes_to_poslen.end(); ++it ) {
			if(B_unique_keys.find(it->first) == B_unique_keys.end() ) {//key is in A intersect B
				IBLT_DEBUG("Adding " << it->first << "to Party A's set");
				cp_sorted_hashes.push_back(it->first);
			} else {
				IBLT_DEBUG(it->first << "only in Party B's set");

			}
		}

		for(auto it = A_unique_keys.begin(); it != A_unique_keys.end(); ++it) {
			cp_sorted_hashes.push_back(*it);
			IBLT_DEBUG("Adding " << *it << "to Party A's set, only in Party A");
		}
		std::sort(cp_sorted_hashes.begin(), cp_sorted_hashes.end());
		IBLT_DEBUG("Party A has" << cp_sorted_hashes.size() << " hashes");

		std::unordered_map<hash_type, size_t> cp_hash_to_index;
		for(size_t i = 0; i < cp_sorted_hashes.size(); ++i) {
			cp_hash_to_index[cp_sorted_hashes[i]] = i;
		}

  		// fill up chunk_exists structure
  		for(size_t i = 0; i < my_rd1.hashes.size(); ++i ) {
  			//chunk exists for both parties if can't find the hash in my unique hashes
  			bool chunk_exists = (B_unique_keys.find(my_rd1.hashes[i].first) == B_unique_keys.end());
  			my_rd2.chunk_exists.push_back( chunk_exists );
  			hash_type hash_val = my_rd1.hashes[i].first;
  			// std::cout << "Current chunk hashval is " << hash_val;
  			if(my_rd2.chunk_exists[i]) { 

  				// if chunk exists, then all we need to do is find the appropriate index in Party A's set of hashes
  				size_t cp_chunk_index = cp_hash_to_index[hash_val];
  				my_rd2.existing_chunk_encoding.push_back(cp_chunk_index);
  			} else { //if chunk doesn't exist, then we need to copy the actual characters from the file
  				size_t chunk_size = my_rd1.hashes_to_poslen[hash_val].second;
  				size_t chunk_pos = my_rd1.hashes_to_poslen[hash_val].first;
  				std::string chunk_info(&buf[chunk_pos], chunk_size);
  				my_rd2.new_chunk_info.push_back(chunk_info);
  			}
  		}

  		delete[] buf;
  	}

  	void reconstruct_file(const char* filename, Round1Info& my_rd1, Round2Info& cp_rd2) {
  		FILE* fp = fopen("temp.txt", "w");
		if( !fp ) {
			cout << "Unable to open file" << endl;
			exit(1);
		}

		char* buf;
		load_buffer_with_file(filename, &buf);

		std::vector<hash_type> ordered_hashes;
		for(auto it = my_rd1.hashes_to_poslen.begin(); it != my_rd1.hashes_to_poslen.end(); ++it) {
			ordered_hashes.push_back(it->first);
		}
		size_t new_chunk = 0;
		size_t existing_chunk = 0;

		for(auto it = cp_rd2.chunk_exists.begin(); it != cp_rd2.chunk_exists.end(); ++it) {
			if(*it) { // if I already have the chunk somewhere, then all I need to do is find it using the encoding
				size_t existing_chunk_hash_index = cp_rd2.existing_chunk_encoding[existing_chunk];
				hash_type relevant_hash = ordered_hashes[existing_chunk_hash_index];
				std::pair<size_t, size_t> chunk_info = my_rd1.hashes_to_poslen[relevant_hash];
				fwrite(&buf[chunk_info.first], 1, chunk_info.second, fp);
				IBLT_DEBUG("I already have chunk starting at pos " << chunk_info.first << " with len " << chunk_info.second);
				++existing_chunk;
			} else { // otherwise I need to read it from the passed in structure
				fwrite(cp_rd2.new_chunk_info[new_chunk].c_str(), 1, cp_rd2.new_chunk_info[new_chunk].size(), fp);
				IBLT_DEBUG("I need new chunk with contents" << cp_rd2.new_chunk_info[new_chunk] << " with len " << cp_rd2.new_chunk_info[new_chunk].size());
				++new_chunk;
			}
		}
		delete[] buf;
		fclose(fp);
  	}
};

#endif
