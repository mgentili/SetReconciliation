#ifndef TABULATION
#define TABULATION

#include <stdint.h>
#include <vector>
#include <random>
#include <iostream>
/**
IMPROVEMENTS TODO:
1) Make vector instead of template? (easier to test different sizes quickly)
**/

template <int key_bits, typename hash_type = uint64_t, typename chunk_type = uint16_t>
class TabulationHashing {
  public:
  	static const int table_first_dim = key_bits/(8*sizeof(chunk_type));
  	static const uint64_t table_second_dim = 1UL << (8*sizeof(chunk_type));
  	hash_type table[table_first_dim][table_second_dim];
  	std::mt19937_64 gen; //mersenne twister for generating random 64-bit integers

    TabulationHashing() {}

  	TabulationHashing( uint64_t s ) {
  		set_seed(s);
  	}

    void set_seed( uint64_t s ) {
      gen.seed(s);

      for(int i = 0; i < table_first_dim; ++i) {
        for( uint64_t j = 0; j < table_second_dim; ++j) {
          table[i][j] = (hash_type) gen();
        }
      }
    }

    //takes in key, casts each block into a chunk that can then be used as in index
    // into the table of random numbers
  	hash_type hash( std::string* key ) const {
      return hash(key->c_str());
  	}

    hash_type hash( const void* key ) const {
      hash_type ret = 0;
      for(int i = 0; i < table_first_dim; ++i) {
        //std::cout << "Next chunk" << ((chunk_type*) key)[i] << ", " << table[i][((chunk_type*) key)[i]] << std::endl;

        //printf("Next chunk: %d, %d\n", ((chunk_type*) key)[i], table[i][((chunk_type*) key)[i]]);
        ret ^= table[i][((chunk_type*) key)[i]];
      }
      return ret;
    }

};

#endif