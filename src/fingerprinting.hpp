#ifndef _FINGERPRINTING
#define _FINGERPRINTING

#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include "MurmurHash2.hpp"

using namespace std;

template <typename hash_type = uint64_t>
class RollingHash {
  public:
  	static const int p = 77711;
  	size_t kgrams;
  	char* buf;
  	size_t size;
  	size_t curr_pos;
  	hash_type last_hash;

  	RollingHash(size_t kgrams): kgrams(kgrams), buf(NULL), size(0), curr_pos(0), last_hash(0) {}
  	~RollingHash() {
  		cleanup();
  	}

  	hash_type myPow(hash_type x, int p)
	{
	  if (p == 0) return 1;
	  if (p == 1) return x;

	  hash_type tmp = myPow(x, p/2);
	  if (p%2 == 0) return tmp * tmp;
	  else return x * tmp * tmp;
	}

  	size_t load_file(const char* filename) {
		FILE* fp = fopen(filename, "r");
		if( !fp ) {
			cout << "Unable to open file" << endl;
		}

		cleanup();

		fseek(fp, 0, SEEK_END); // seek to end of file
		size = ftell(fp); // get current file pointer
		fseek(fp, 0, SEEK_SET); // seek back to beginning of file
		buf = new char[size];
		fread(buf, 1, size, fp);
		fclose(fp);
		return size;
	}

	void cleanup() {
		delete[] buf;
		curr_pos = 0;
		last_hash = 0;
		size = 0;
	}

	hash_type next_hash() {
		hash_type ret;
		if( curr_pos < kgrams )
			ret = hash(buf, curr_pos);
		else
			ret = hash(last_hash, buf[curr_pos-kgrams], buf[curr_pos]);
		last_hash = ret;
		curr_pos++;
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
		//std::cout << "old char: " << prevc << " new char: " << newc << "pow" << myPow(p, kgrams) << endl;  
		//return( ((curr_hash - prevc*myPow(p, kgrams)) + newc)*p );
		//return (curr_hash - prevc*myPow(p, kgrams-1))*p + newc;
		return curr_hash*p - prevc*myPow(p, kgrams) + newc;
	}
};

/** Fingerprinter is used to generate sets of (hash, file_pos) pairs that are a small 
 ** representation of the file. It can further digest those pairs to create a set of hashes that represents the file
 **/
template <typename hash_type = uint64_t, typename hasher_type = RollingHash<hash_type> >
class Fingerprinter {
  public:
  	size_t kgrams;
  	size_t w;
  	hash_type* h;
  	hasher_type hasher;

  	Fingerprinter(size_t kgrams, size_t window_size): kgrams(kgrams), w(window_size), hasher(kgrams) {}

  	// uses winnowing to determine set of hashes for file
  	// returns the size of the hashed file
	size_t winnow(const char* filename, vector<pair<hash_type, int> >& hashes) {
		h = new hash_type[w];

		for(size_t i = 0; i < w; ++i) {
			h[i] = (hash_type) (-1);
		}

		size_t file_size = hasher.load_file(filename);
		int min = 0; //index of minimal hash in window
		//cout << "File size" << hasher.size << endl;
		for(size_t r = 1; r < file_size - w; ++r) {
			h[r % w] = hasher.next_hash();
			//std::cout << ",h: " << h[r % w] << std::endl;
			if( (min % w) == (r % w) ) {
				/* the previous minimum is no longer in this window. Scan h leftward
				 * starting from r for the rightmost hash for the rightmost minimal hash. 
				 * Note min starts with the index of the rightmost hash 
				 */
				//if( hashes.size() > 0) 
				//		std::cout << "Old hash" << hashes.back().first << "pos, " << hashes.back().second;
				for( size_t j = 1; j < w; ++j ) {
					if( h[(r - j + w) % w] < h[min % w] ) {
						//std::cout << "Smaller hash" << h[(r-j+w)%w] << std::endl;
						min = r - j;
					}
				}
				if( (min % w) == (r % w )) /*account for case where none smaller than new hash*/
					min = r;
				hashes.push_back(make_pair(h[min % w], min));
				//std::cout << "New hash" << hashes.back().first << "pos, " << hashes.back().second << std::endl;


			} else {
				if( h[r % w] <= h[min % w] ) {
					min = r;
					hashes.push_back(make_pair(h[min % w], min));
				}
			}
		}
		delete[] h;
		return file_size;
	}

	size_t modding(const char* filename, int p, vector<pair<hash_type, int> >& hashes) {
		size_t file_size = hasher.load_file(filename);
		for(size_t i = 0; i < file_size; ++i) {
			hash_type next_hash = hasher.next_hash();
			if( next_hash % p == 0) {
				hashes.push_back(make_pair(next_hash, i));
			}
		}
		return file_size;
	}

	size_t get_fingerprint(const char* filename, vector<pair<hash_type, int> >& hashes) {
		
		return winnow(filename, hashes);
		//return modding(filename, 10, hashes);
	}

	size_t digest_file(const char* filename, vector<pair<hash_type, int> >& file_hashes) {
		vector<pair<hash_type, int> > fp_hashes;
		size_t file_size = winnow(filename, fp_hashes);
		fp_hashes.push_back(make_pair(-1, file_size)); // add placeholder for end of file
		FILE* fp = fopen(filename, "r");
		if( !fp ) {
			cout << "Unable to open file" << endl;
		}
		char* buf = new char[file_size];
		fread(buf, 1, file_size, fp);
		fclose(fp);

		std::pair<hash_type, int> curr_pair;
		size_t i = 0;
		size_t curr_pos = 0;
		for(; i < fp_hashes.size(); ++i) {	
			size_t curr_len = fp_hashes[i].second - curr_pos;
			// std::cout << "Abs pos" << fp_hashes[i].second << " Curr len" << curr_len << " Curr pos" << curr_pos << std::endl;
			curr_pair = make_pair(MurmurHash64A(&buf[curr_pos], curr_len, 0), curr_len);
			curr_pos += curr_len;
			file_hashes.push_back(curr_pair);
		}

		return file_size;
	}

	// TODO: implement
	size_t two_way_min(const char* filename, vector<pair<hash_type, int> >& hashes) {
		h = new hash_type[w];

		for(size_t i = 0; i < w; ++i) {
			h[i] = (hash_type) (-1);
		}

		hasher.load_file(filename);
		return -1;
	}

};

#endif