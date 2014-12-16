#ifndef _FILE_SYNC
#define _FILE_SYNC

#include <map>
#include <assert.h>
#include "multiIBLT.hpp"
#include "fingerprinting.hpp"
#include "IBLT_helpers.hpp"

template <size_t n_parties = 2, typename hash_type = uint64_t>
class FileSynchronizer {
  public:
  	const size_t kgrams = 10;
  	const size_t window_size = 100;
  	const size_t num_buckets = 1000;
  	const size_t num_hashfns = 4;
  	typedef multiIBLT<n_parties, hash_type> iblt_type;

	class Round1Info {
	  public:
	  	typedef multiIBLT<n_parties, hash_type> iblt_type;
	  	std::vector<pair<hash_type, size_t> > hashes; //hash and length
	  	std::map<hash_type, std::pair<size_t, size_t> > hashes_to_poslen; //these don't actually need to be transferred, since can be reconstructed from hashes
	  	iblt_type* iblt;

	  	Round1Info() {};
	};

	class Round2Info {
	  public:
		std::vector<bool> chunk_exists; 
		std::vector<bool> hash_exists;
		std::vector<std::pair<size_t, char*> > new_chunk_info; //length and contents
		std::vector<size_t> existing_chunk_encoding; //
		Round2Info() {};
	};

  	FileSynchronizer() {};

  	void send_IBLT(const char* filename, Round1Info& my_rd1) {
		Fingerprinter<hash_type> f(kgrams, window_size);
		f.digest_file( filename, my_rd1.hashes);

		my_rd1.iblt = new iblt_type(num_buckets, num_hashfns);
		for(auto it = my_rd1.hashes.begin(); it != my_rd1.hashes.end(); ++it) {
			my_rd1.iblt->insert_key(it->first);
			
		}

		// create mapping from my hashes to their block lengths and start position in file
		size_t curr_pos = 0;
  		for(auto it = my_rd1.hashes.begin(); it != my_rd1.hashes.end(); ++it) {
  			if( my_rd1.hashes_to_poslen.find(it->first) != my_rd1.hashes_to_poslen.end() ) {
  				std::cout << "Already have hash " << it->first << " pos " << curr_pos << " len" << it->second << std::endl;
  			}
  			my_rd1.hashes_to_poslen[it->first] = make_pair(curr_pos, it->second);
			curr_pos += it->second;
  			std::cout << "File" << filename << " Hash " << it->first << " pos " << curr_pos << " len" << it->second << std::endl;

  		}
  		std::cout << "File " << filename << " has " << my_rd1.hashes.size() << "hashes" << ". hashes_to_pos_len has size" << my_rd1.hashes_to_poslen.size() << std::endl;
  	}

  	void receive_IBLT(const char* filename, Round1Info& my_rd1, iblt_type& cp_IBLT, Round2Info& my_rd2) {
  		iblt_type resIBLT(num_buckets, num_hashfns);
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
  			if( my_rd1.hashes_to_poslen.find(it->first) != my_rd1.hashes_to_poslen.end() ) {
  				B_unique_keys.insert(it->first);
  			} else {
  				A_unique_keys.insert(it->first);
  			}
  		}

  		char* buf;
		load_buffer_with_file(filename, &buf);

		std::vector<hash_type> cp_sorted_hashes;
		//Create structure of Party A's sorted hashes
		for(auto it = my_rd1.hashes_to_poslen.begin(); it != my_rd1.hashes_to_poslen.end(); ++it ) {
			if(B_unique_keys.find(it->first) == B_unique_keys.end() ) {//key is in A intersect B
				std::cout << "Adding " << it->first << "to Party A's set" << std::endl;
				cp_sorted_hashes.push_back(it->first);
			} else {
				std::cout << it->first << "only in Party B's set" << std::endl;

			}
		}

		for(auto it = A_unique_keys.begin(); it != A_unique_keys.end(); ++it) {
			cp_sorted_hashes.push_back(*it);
			std::cout << "Adding " << *it << "to Party A's set, only in Party A" << std::endl;
		}
		std::sort(cp_sorted_hashes.begin(), cp_sorted_hashes.end());
		std::cout << "Party A has" << cp_sorted_hashes.size() << " hashes" << std::endl;

		std::unordered_map<hash_type, size_t> cp_hash_to_index;
		for(size_t i = 0; i < cp_sorted_hashes.size(); ++i) {
			cp_hash_to_index[cp_sorted_hashes[i]] = i;
			std::cout << "Hashmap" << cp_sorted_hashes[i] << "has index" << cp_hash_to_index[cp_sorted_hashes[i]] << std::endl;
		}
  		// fill up chunk_exists structure
  		for(size_t i = 0; i < my_rd1.hashes.size(); ++i ) {
  			//chunk exists for both parties if can't find the hash in my unique hashes
  			bool chunk_exists = (B_unique_keys.find(my_rd1.hashes[i].first) == B_unique_keys.end());
  			my_rd2.chunk_exists.push_back( chunk_exists );
  			hash_type hash_val = my_rd1.hashes[i].first;
  			std::cout << "Current chunk hashval is " << hash_val;
  			if(my_rd2.chunk_exists[i]) { 

  				// if chunk exists, then all we need to do is find the appropriate index in Party A's set of hashes
  				size_t cp_chunk_index = cp_hash_to_index[hash_val];
  				my_rd2.existing_chunk_encoding.push_back(cp_chunk_index);
  				std::cout << "Chunk" << i << "exists with pos" << my_rd1.hashes_to_poslen[hash_val].first << "and size " << 
  				my_rd1.hashes_to_poslen[hash_val].second << std::endl;
  			} else { //if chunk doesn't exist, then we need to copy the actual characters from the file
  				size_t chunk_size = my_rd1.hashes_to_poslen[hash_val].second;
  				size_t chunk_pos = my_rd1.hashes_to_poslen[hash_val].first;
  				char* chunk_info = new char[chunk_size];
  				memcpy(chunk_info, &buf[chunk_pos], chunk_size);
  				my_rd2.new_chunk_info.push_back(make_pair(chunk_size, chunk_info));
  				std::cout << "Chunk" << i << "doesn't exist with len, pos" << chunk_size << 
  				"," << chunk_pos << "and info" << chunk_info << std::endl;

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
			std::cout << "Party A has hash" << it->first << std::endl;

		}
		size_t new_chunk = 0;
		size_t existing_chunk = 0;


		for(auto it = cp_rd2.chunk_exists.begin(); it != cp_rd2.chunk_exists.end(); ++it) {
			if(*it) { // if I already have the chunk somewhere, then all I need to do is find it using the encoding
				size_t existing_chunk_hash_index = cp_rd2.existing_chunk_encoding[existing_chunk];
				hash_type relevant_hash = ordered_hashes[existing_chunk_hash_index];
				std::pair<size_t, size_t> chunk_info = my_rd1.hashes_to_poslen[relevant_hash];
				fwrite(&buf[chunk_info.first], 1, chunk_info.second, fp);
				std::cout << "I already have chunk starting at pos " << chunk_info.first << " with len " << chunk_info.second << std::endl;
				++existing_chunk;
			} else { // otherwise I need to read it from the passed in structure
				fwrite(cp_rd2.new_chunk_info[new_chunk].second, 1, cp_rd2.new_chunk_info[new_chunk].first, fp);
				std::cout << "I need new chunk with contents" << cp_rd2.new_chunk_info[new_chunk].second << " with len " << cp_rd2.new_chunk_info[new_chunk].first << std::endl;
				++new_chunk;
			}
		}
		delete[] buf;
		fclose(fp);
  	}
};

#endif
