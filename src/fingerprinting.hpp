#ifndef _FINGERPRINTING
#define _FINGERPRINTING

#include <assert.h>
#include <stdio.h>

#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#include "hash_util.hpp"
#include "IBLT_helpers.hpp"

#define FINGERPRINT_DEBUG 1

template <typename hash_type = uint64_t>
class RollingHash {
  public:
  	static const uint32_t p = 77711;
  	size_t kgrams;
  	std::vector<char> buf;
  	size_t size;
  	size_t curr_pos;
  	hash_type last_hash;

  	RollingHash(size_t kgrams): kgrams(kgrams), size(0), curr_pos(0), last_hash(0) {}
  	~RollingHash() {
  		cleanup();
  	}

  	uint64_t myPow(uint32_t x, int power)
	{
	  if (power == 0) return 1;
	  if (power == 1) return x;

	  hash_type tmp = myPow(x, power/2);
	  if (power%2 == 0) return tmp * tmp;
	  else return x * tmp * tmp;
	}

  	size_t load_file(const std::string& filename) {
  		cleanup();
  		return( load_buffer_with_file(filename, buf) );
	}

	void cleanup() {
		buf.clear();
		curr_pos = 0;
		last_hash = 0;
		size = 0;
	}

	hash_type next_hash() {
		hash_type ret;
		if( curr_pos < kgrams )
			ret = hash(buf.data(), curr_pos);
		else
			ret = hash(last_hash, buf[curr_pos-kgrams], buf[curr_pos]);
		last_hash = ret;
		++curr_pos;
		return ret;
	}

	// for first few hashes (before we have a full kgram)
	hash_type hash(char* substr, size_t n) {
		hash_type ret = 0;
		assert( n <= kgrams - 1);
		for(size_t i = 0; i <= n; ++i) {
			ret *= p;
			ret += substr[i];
		}
		return ret;
	}

	// once we have full k-gram, can use faster method
	hash_type hash(hash_type curr_hash, char prevc, char newc) {
		return (hash_type) (curr_hash*p - prevc*myPow(p, kgrams) + newc);
	}
};

/** Fingerprinter is used to generate sets of (hash, file_pos) pairs that are a small 
 ** representation of the file. It can further digest those pairs to create a 
 ** set of hashes that represents the file
 **/
template <typename hash_type = uint64_t, typename hasher_type = RollingHash<hash_type> >
class Fingerprinter {
  public:
	const size_t kgrams = 10;
  	size_t avg_block_size;
    std::vector<hash_type> h;
  	hasher_type hasher;
	
	Fingerprinter(size_t block_size): avg_block_size(block_size), hasher(kgrams) {}

  	// uses winnowing to determine set of hashes for file
  	// returns the size of the hashed file
	size_t winnow(const std::string& filename, 
                  std::vector<std::pair<hash_type, size_t> >& hashes) {
		size_t w = 2*avg_block_size - 1;
		
		h.resize(w);

		for(size_t i = 0; i < w; ++i) {
			h[i] = (hash_type) (-1);
		}

		size_t file_size = hasher.load_file(filename);
		
        int min = 0; //index of minimal hash in window
		for(size_t r = 1; r < file_size - w; ++r) {
			h[r % w] = hasher.next_hash();
			if( (min % w) == (r % w) ) {
				/* the previous minimum is no longer in this window. Scan h leftward
				 * starting from r for the rightmost hash for the rightmost minimal hash. 
				 * Note min starts with the index of the rightmost hash 
				 */
				for( size_t j = 1; j < w; ++j ) {
					if( h[(r - j + w) % w] < h[min % w] ) {
						min = r - j;
					}
				}
				if( (min % w) == (r % w )) /*account for case where none smaller than new hash*/
					min = r;
				hashes.push_back(std::make_pair(h[min % w], min));
			} else {
				if( h[r % w] <= h[min % w] ) {
					min = r;
					hashes.push_back(std::make_pair(h[min % w], min));
				}
			}
		}
		return file_size;
	}

	size_t modding(const std::string& filename, 
                   std::vector<std::pair<hash_type, size_t> >& hashes) {
		size_t p = avg_block_size;
		size_t file_size = hasher.load_file(filename);
		for(size_t i = 0; i < file_size; ++i) {
			hash_type next_hash = hasher.next_hash();
			if( next_hash % p == 0) {
				hashes.push_back(std::make_pair(next_hash, i));
			}
		}
		return file_size;
	}

	size_t get_fingerprint(const std::string& filename, 
                           std::vector<std::pair<hash_type, size_t> >& hashes) {
		return winnow(filename, hashes);
	}

	size_t digest_file(const std::string& filename, 
                       std::vector<std::pair<hash_type, size_t> >& file_hashes) {
		return digest_file(filename, file_hashes, 0);
	}

	// processes a file, returning a set of hashes of blocks and the corresponding block lengths
	size_t digest_file(const std::string& filename, 
                       std::vector<std::pair<hash_type, size_t> >& file_hashes, 
                       size_t overlap_len) {
        std::vector<std::pair<hash_type, size_t> > fp_hashes;
		size_t file_size = get_fingerprint(filename, fp_hashes);
		fp_hashes.push_back(std::make_pair(-1, file_size)); // add placeholder for end of file
		
		std::vector<char> buf;
		load_buffer_with_file(filename, buf);

		std::pair<hash_type, size_t> curr_pair;
#if FINGERPRINT_DEBUG
		std::unordered_map<hash_type, std::string> hash_to_string;
#endif
		size_t i = 0;
		size_t curr_pos = 0;
		for(; i < fp_hashes.size(); ++i) {	
			size_t curr_len = fp_hashes[i].second - curr_pos;
			size_t len_plus_offset = 
                (i == (fp_hashes.size() - 1)) 
                    ? curr_len 
                    : (curr_len + overlap_len);
			curr_pair = std::make_pair(
                            HashUtil::MurmurHash64A(&buf[curr_pos], len_plus_offset, 0),
                            curr_len);
#if FINGERPRINT_DEBUG
			std::string curr_string(&buf[curr_pos], curr_len);
			if( hash_to_string.find(curr_pair.first) != hash_to_string.end() 
                && hash_to_string[curr_pair.first] != curr_string) {
				std::cout << "Uh oh, different contents have the same hash" \
                          << "need to use a hash with more bytes" << std::endl;
				std::cout << "Same hash for " << curr_string 
                          << " and " << hash_to_string[curr_pair.first] << std::endl;
				exit(1);
			}
			hash_to_string[curr_pair.first] = curr_string;
#endif
			file_hashes.push_back(curr_pair);
			curr_pos += curr_len;
		}
		buf.clear();
		hasher.cleanup();
		return file_size;
	}

	// TODO: implement
	size_t two_way_min(const std::string& filename, 
                       std::vector<std::pair<hash_type, size_t> >& hashes) {
		return -1;
	}

};

#endif
