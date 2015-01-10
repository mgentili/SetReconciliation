#ifndef _FILE_SYNC
#define _FILE_SYNC

#include <map>
#include <assert.h>
#include "multiIBLT.hpp"
#include "fingerprinting.hpp"
#include "IBLT_helpers.hpp"
#include "file_sync.pb.h"

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
		std::vector<bool> chunk_exists;  //for each chunk server has, whether client has
		std::vector<bool> hash_exists; //for each hash client has, whether server has
		std::vector<std::string> new_chunk_info; //length and contents of each chunk that client doesn't have
		std::vector<size_t> existing_chunk_encoding; //index of chunk hash within hash_exists
		Round2Info() {};

		size_t size_in_bits() { // in bits
			size_t tot_bits = 0;
			tot_bits += chunk_exists.size();
			tot_bits += hash_exists.size();
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

			for(auto it = hash_exists.begin(); it != hash_exists.end(); ++it) {
				rd2.add_hash_exists(*it);
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

			for(int i = 0; i < rd2.hash_exists_size(); ++i) {
				hash_exists.push_back(rd2.hash_exists(i));
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

  	void send_IBLT(const char* filename, Round1Info& my_rd1) {
		Fingerprinter<hash_type> f(kgrams, window_size);
		f.digest_file( filename, my_rd1.hashes);

		// create mapping from my hashes to their block lengths and start position in file
		size_t curr_pos = 0;
  		for(auto it = my_rd1.hashes.begin(); it != my_rd1.hashes.end(); ++it) {
  			if( my_rd1.hashes_to_poslen.find(it->first) != my_rd1.hashes_to_poslen.end() ) {
  				std::cout << "Already have hash " << it->first << " pos " << curr_pos << " len" << it->second << std::endl;
  			}
  			my_rd1.hashes_to_poslen[it->first] = make_pair(curr_pos, it->second);
  			std::cout << "File" << filename << " Hash " << it->first << " pos " << curr_pos << " len" << it->second << std::endl;
			curr_pos += it->second;
  		}
  		std::cout << "File " << filename << " has " << my_rd1.hashes.size() << "hashes" << ". hashes_to_pos_len has size" << my_rd1.hashes_to_poslen.size() << std::endl;
  	
  		my_rd1.iblt = new iblt_type(num_buckets, num_hashfns);
		for(auto it = my_rd1.hashes_to_poslen.begin(); it != my_rd1.hashes_to_poslen.end(); ++it) {
			my_rd1.iblt->insert_key(it->first);
		}
  	}

  	void receive_IBLT(const char* filename, Round1Info& my_rd1, iblt_type& cp_IBLT, Round2Info& my_rd2, Round1Info& cp_rd1) {
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
  			std::cout << "Distinct key " << it->first << std::endl;
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
			// std::cout << "Hashmap" << cp_sorted_hashes[i] << "has index" << cp_hash_to_index[cp_sorted_hashes[i]] << std::endl;
		}

		for(auto it = cp_rd1.hashes.begin(); it != cp_rd1.hashes.end(); ++it) {
			if( cp_hash_to_index.find(it->first) == cp_hash_to_index.end() ) {
				std::cout << "Couldn't find hash" << it->first << std::endl;
			}
		}

		for(auto it = cp_hash_to_index.begin(); it != cp_hash_to_index.end(); ++it) {
			if( cp_rd1.hashes_to_poslen.find(it->first) == cp_rd1.hashes_to_poslen.end() ) {
				std::cout << "Couldn't find hash" << it->first << std::endl;
			}
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
  				// std::cout << "Chunk" << i << "exists with pos" << my_rd1.hashes_to_poslen[hash_val].first << "and size " << 
  				// my_rd1.hashes_to_poslen[hash_val].second << std::endl;
  			} else { //if chunk doesn't exist, then we need to copy the actual characters from the file
  				size_t chunk_size = my_rd1.hashes_to_poslen[hash_val].second;
  				size_t chunk_pos = my_rd1.hashes_to_poslen[hash_val].first;
  				std::string chunk_info(&buf[chunk_pos], chunk_size);
  				//char* chunk_info = new char[chunk_size];
  				//memcpy(chunk_info, &buf[chunk_pos], chunk_size);
  				//my_rd2.new_chunk_info.push_back(make_pair(chunk_size, chunk_info));
  				my_rd2.new_chunk_info.push_back(chunk_info);
  				// std::cout << "Chunk" << i << "doesn't exist with len, pos" << chunk_size << 
  				// "," << chunk_pos << "and info" << chunk_info << std::endl;

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
			// std::cout << "Party A has hash" << it->first << std::endl;

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
				fwrite(cp_rd2.new_chunk_info[new_chunk].c_str(), 1, cp_rd2.new_chunk_info[new_chunk].size(), fp);
				std::cout << "I need new chunk with contents" << cp_rd2.new_chunk_info[new_chunk] << " with len " << cp_rd2.new_chunk_info[new_chunk].size() << std::endl;
				++new_chunk;
			}
		}
		delete[] buf;
		fclose(fp);
  	}
};

#endif
