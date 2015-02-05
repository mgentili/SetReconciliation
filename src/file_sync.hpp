#ifndef _FILE_SYNC
#define _FILE_SYNC

#include <map>
#include <assert.h>
#include <zlib.h>
#include <cmath>

#include "basicIBLT.hpp"
#include "multiIBLT.hpp"
#include "fingerprinting.hpp"
#include "IBLT_helpers.hpp"
#include "file_sync.pb.h"
#include "StrataEstimator.hpp"
#include "compression.hpp"

#define FILE_SYNC_DEBUG 0
#if FILE_SYNC_DEBUG
#  define SYNC_DEBUG(x)  do { std::cerr << x << std::endl; } while(0)
#else
#  define SYNC_DEBUG(x)  do {} while (0)
#endif

#define FILE_ENCODING_DEBUG 0
#if FILE_ENCODING_DEBUG
#  define ENCODING_DEBUG(x)  do { std::cerr << x << std::endl; } while(0)
#else
#  define ENCODING_DEBUG(x)  do {} while (0)
#endif

#define DEFAULT_BLOCK_SIZE 700

template <typename hash_type = uint32_t, typename iblt_type = basicIBLT<hash_type, hash_type> >
class FileSynchronizer {
  public:
  	class Round1Info;
  	class Round2Info;

  	Round1Info my_rd1;
  	Round2Info my_rd2;
  	std::unordered_set<hash_type> my_distinct_keys, cp_distinct_keys;
  	std::string file;
	size_t avg_block_size; 

  	FileSynchronizer(std::string& filename): file(filename)  {
		avg_block_size = get_block_size( get_file_size(file.c_str() ) );
		process_file(file);
  	};

	FileSynchronizer(std::string& filename, size_t avg_block_size): file(filename), avg_block_size(avg_block_size) {
		process_file(file);
	} 

//ENCODING STUFF:
  	std::string send_strata_encoding() {
  		file_sync::strata_estimator estimator;
  		my_rd1.estimator.serialize(estimator);
  		std::string estimator_encoding;
  		estimator.SerializeToString(&estimator_encoding);
		ENCODING_DEBUG("Serialized estimator structure: " << estimator_encoding.size()*8 << " bits vs actual " << my_rd1.estimator.size_in_bits());
		estimator_encoding = compress_string(estimator_encoding);
		ENCODING_DEBUG("Compressed estimator structure: " << estimator_encoding.size()*8 << " bits");
  		return estimator_encoding;
  	}

  	size_t receive_strata_encoding(const std::string& strata_encoding) {
  		file_sync::strata_estimator estimator;
  		std::string strata_decoding = decompress_string(strata_encoding);
  		estimator.ParseFromString(strata_decoding);
  		StrataEstimator<hash_type> cp_estimator;
  		cp_estimator.deserialize(estimator);
  		size_t diff_estimate = get_difference_estimate(cp_estimator);
		create_IBLT(diff_estimate);
  		return diff_estimate;
  	}

  	std::string send_IBLT_encoding(size_t diff_estimate) {
		create_IBLT(diff_estimate);
  		create_IBLT(diff_estimate);
  		file_sync::IBLT2 iblt_protobuf;
		my_rd1.iblt->serialize(iblt_protobuf);
		std::string iblt_encoding;
		iblt_protobuf.SerializeToString(&iblt_encoding);
		ENCODING_DEBUG("Serialized iblt structure: " << iblt_encoding.size()*8 << " bits vs actual " << (my_rd1.iblt)->size_in_bits());
		iblt_encoding = compress_string(iblt_encoding);
		ENCODING_DEBUG("Compressed iblt structure: " << iblt_encoding.size()*8 << " bits");
		return iblt_encoding;
  	}

  	std::string receive_IBLT_encoding(const std::string& iblt_encoding) {
  		file_sync::IBLT2 iblt_protobuf;
  		std::string iblt_deencoding = decompress_string(iblt_encoding);
		iblt_protobuf.ParseFromString(iblt_deencoding);
		iblt_type new_iblt(my_rd1.iblt->num_buckets, my_rd1.iblt->num_hashfns);
		new_iblt.deserialize(iblt_protobuf);
		receive_IBLT(new_iblt);
		return send_rd2_encoding();
  	}

  	std::string send_rd2_encoding() {
  		file_sync::Round2 rd2_protobuf;
		my_rd2.serialize(rd2_protobuf);
		std::string rd2_encoding;
		rd2_protobuf.SerializeToString(&rd2_encoding);
		ENCODING_DEBUG("Serialized rd2 structure: " << rd2_encoding.size()*8 << " bits vs actual " << my_rd2.size_in_bits());
		rd2_encoding = compress_string(rd2_encoding);
		ENCODING_DEBUG("Compressed rd2 structure: " << rd2_encoding.size()*8 << " bits");
		return rd2_encoding;
  	}

  	void receive_rd2_encoding(const std::string& rd2_encoding) {
		file_sync::Round2 rd2_protobuf;
		std::string rd2_decoding = decompress_string(rd2_encoding);
		rd2_protobuf.ParseFromString(rd2_decoding);
		Round2Info cp_rd2;
		cp_rd2.deserialize(rd2_protobuf);
		reconstruct_file(cp_rd2);
  	}

//PROTOCOL STUFF

	size_t get_block_size(size_t file_size) {
		size_t new_block_size = (size_t) sqrt( file_size );
		return (new_block_size > DEFAULT_BLOCK_SIZE) ? new_block_size : DEFAULT_BLOCK_SIZE;
	}
  	//Party A fills his structure with info to estimate set difference
  	//and fills the rest of the hash structure while he's at it
  	void process_file(const std::string& filename) {

 		Fingerprinter<hash_type> f(avg_block_size);
		f.digest_file( filename.c_str(), my_rd1.hashes);

		// create mapping from my hashes to their block lengths and start position in file
		size_t curr_pos = 0;
  		for(auto it = my_rd1.hashes.begin(); it != my_rd1.hashes.end(); ++it) {
  			if( my_rd1.hashes_to_poslen.find(it->first) != my_rd1.hashes_to_poslen.end() ) {
  				SYNC_DEBUG("Already have hash " << it->first << " pos " << curr_pos << " len" << it->second);
  			} else {
  				my_rd1.hashes_to_poslen[it->first] = make_pair(curr_pos, it->second);
  			}
  			SYNC_DEBUG("File" << filename << " Hash " << it->first << " pos " << curr_pos << " len" << it->second);
			curr_pos += it->second;
  		}
  		SYNC_DEBUG("File " << filename << " has " << my_rd1.hashes.size() << "hashes" << ". hashes_to_pos_len has size" << my_rd1.hashes_to_poslen.size());
  	 	for(auto it = my_rd1.hashes_to_poslen.begin(); it != my_rd1.hashes_to_poslen.end(); ++it) {
  	 		my_rd1.estimator.insert_key(it->first);
  	 	}
  	}

  	size_t get_difference_estimate(StrataEstimator<hash_type>& cp_estimator) {
  		return my_rd1.estimator.estimate_diff(cp_estimator);
  	}

  	void create_IBLT(size_t bucket_estimate) {
  		size_t num_buckets = bucket_estimate * 2;
  		// size_t num_hashfns = ( bucket_estimate < 200 ) ? 3 : 4;
  		size_t num_hashfns = 4;
  		
  		my_rd1.iblt = new iblt_type(num_buckets, num_hashfns);
		fill_IBLT();		
  	}

  	void fill_IBLT() {
  		for(auto it = my_rd1.hashes_to_poslen.begin(); it != my_rd1.hashes_to_poslen.end(); ++it) {
			my_rd1.iblt->insert_key(it->first);
		}
  	}

  	void get_counterparty_hashes(std::vector<hash_type>& cp_sorted_hashes) {
		//Create structure of Party A's sorted hashes by going through each of Party B's hashes
		//and seeing if it is in B-A. If not, then must be in A intersect B
		for(auto it = my_rd1.hashes_to_poslen.begin(); it != my_rd1.hashes_to_poslen.end(); ++it ) {
			if(my_distinct_keys.find(it->first) == my_distinct_keys.end() ) {//key is in A intersect B
				cp_sorted_hashes.push_back(it->first);
			}
		}

		for(auto it = cp_distinct_keys.begin(); it != cp_distinct_keys.end(); ++it) {
			cp_sorted_hashes.push_back(*it);
			// SYNC_DEBUG("Adding " << *it << "to Party A's set, only in Party A");
		}
		std::sort(cp_sorted_hashes.begin(), cp_sorted_hashes.end());
		SYNC_DEBUG("Party A has" << cp_sorted_hashes.size() << " hashes");
  	}

  	bool get_distinct_keys(iblt_type& cp_IBLT) {
  		iblt_type resIBLT(cp_IBLT.num_buckets, cp_IBLT.num_hashfns);
  		resIBLT.add(*(my_rd1.iblt));
  		resIBLT.remove(cp_IBLT);
  		bool res = resIBLT.peel(my_distinct_keys, cp_distinct_keys);
		if( !res ) {
  			std::cerr << "Failed to peel, need to retry" << std::endl;
  			return false;
			//exit(1);
  		}
		return true;
  	}

  	void determine_chunk_encoding(std::vector<hash_type>& cp_sorted_hashes) {
  		char* buf;
		load_buffer_with_file(file.c_str(), &buf);

  		std::unordered_map<hash_type, size_t> cp_hash_to_index;
		for(size_t i = 0; i < cp_sorted_hashes.size(); ++i) {
			cp_hash_to_index[cp_sorted_hashes[i]] = i;
		}

  		// fill up chunk_exists structure
  		for(size_t i = 0; i < my_rd1.hashes.size(); ++i ) {
  			//chunk exists for both parties if can't find the hash in my unique hashes
  			bool chunk_exists = (my_distinct_keys.find(my_rd1.hashes[i].first) == my_distinct_keys.end());
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

  	bool receive_IBLT(iblt_type& cp_IBLT) {
  	// void receive_IBLT(const char* filename, iblt_type& cp_IBLT, Round1Info& cp_rd1) {
		
		if( !get_distinct_keys(cp_IBLT) ) {
			return false;
		};

		std::vector<hash_type> cp_sorted_hashes;
		get_counterparty_hashes(cp_sorted_hashes);

		determine_chunk_encoding(cp_sorted_hashes);
		return true;
  	}

  	
  	void reconstruct_file(Round2Info& cp_rd2) {
  		FILE* fp = fopen("tmp/temp.txt", "w");
		if( !fp ) {
			cout << "Unable to open file" << endl;
			exit(1);
		}

		char* buf;
		load_buffer_with_file(file.c_str(), &buf);

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
				SYNC_DEBUG("I already have chunk starting at pos " << chunk_info.first << " with len " << chunk_info.second);
				++existing_chunk;
			} else { // otherwise I need to read it from the passed in structure
				fwrite(cp_rd2.new_chunk_info[new_chunk].c_str(), 1, cp_rd2.new_chunk_info[new_chunk].size(), fp);
				SYNC_DEBUG("I need new chunk with contents" << cp_rd2.new_chunk_info[new_chunk] << " with len " << cp_rd2.new_chunk_info[new_chunk].size());
				++new_chunk;
			}
		}
		delete[] buf;
		fclose(fp);
  	}
};

template <typename hash_type, typename iblt_type>
class FileSynchronizer<hash_type, iblt_type>::Round1Info {
  public:
	StrataEstimator<hash_type> estimator;
  	std::vector<pair<hash_type, size_t> > hashes; //hash and length
  	std::map<hash_type, std::pair<size_t, size_t> > hashes_to_poslen; //mapping from hash to position in file and length
  	iblt_type* iblt;

  	~Round1Info() {
  		delete iblt;
  	}
};

template <typename hash_type, typename iblt_type>
class FileSynchronizer<hash_type, iblt_type>::Round2Info { //info to update A to have the same file as B
  public:
	std::vector<bool> chunk_exists;  //for each chunk B has, whether A has
	std::vector<std::string> new_chunk_info; //length and contents of each chunk that A doesn't have
	std::vector<uint32_t> existing_chunk_encoding; //index of chunk hash within client's 

	size_t size_in_bits() { // in bits
		size_t tot_bits = 0;
		tot_bits += chunk_exists.size();
		for(auto it = new_chunk_info.begin(); it != new_chunk_info.end(); ++it) {
			tot_bits += it->size()*8;
		}
		tot_bits += existing_chunk_encoding.size() * sizeof(uint32_t)* 8;
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

	void print_size_info() const {
		std::cout << "Chunk exists has            " << chunk_exists.size() << " bools" << std::endl;
		std::cout << "New chunk info has          " << new_chunk_info.size() << " strings" << std::endl;
		std::cout << "Existing chunk encoding has " << existing_chunk_encoding.size() << " uint32_ts" << std::endl;
	}
};


#endif
