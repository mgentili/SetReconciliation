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
    //hash_type table[table_first_dim][table_second_dim];
    hash_type** table;
    std::mt19937_64 gen; //mersenne twister for generating random 64-bit integers

    TabulationHashing() {
      setup_table();
      set_seed(0);
    }

    TabulationHashing( uint64_t s ) {
      setup_table();
      set_seed(s);
    }

    ~TabulationHashing() {
      for(int i = 0; i < table_first_dim; ++i) {
        delete[] table[i];
      }
      delete[] table;
    }

    void setup_table() {
      table = new hash_type*[table_first_dim];
      for(int i = 0; i < table_first_dim; ++i) {
        table[i] = new hash_type[table_second_dim];
      }
    }

    void set_seed( uint64_t s ) {
      gen.seed(s);

      for(int i = 0; i < table_first_dim; ++i) {
        for( uint64_t j = 0; j < table_second_dim; ++j) {
          table[i][j] = (hash_type) gen();
        }
      }
    }

    hash_type hash( const uint64_t& key ) const {
      hash_type ret = 0;
      uint64_t tmp = key;
      for(int i = 0; i < table_first_dim; ++i) {
        chunk_type curr_chunk = (chunk_type) tmp;
        ret ^= table[i][curr_chunk];
        tmp = tmp >> (8*sizeof(chunk_type));
      }
      return ret;
    }

    //takes in key, casts each block into a chunk that can then be used as in index
    // into the table of random numbers
    hash_type hash( const std::string& key ) const {
      return hash(key.c_str());
    }

    hash_type hash( const char* key) const {
      hash_type ret = 0;
      for(int i = 0; i < table_first_dim; ++i) {
        ret ^= table[i][((chunk_type*) key)[i]];
      }
      return ret;
    }
};

// template <typename key_type, int key_bits, typename hash_type = uint64_t, typename chunk_type = uint16_t>
// class TabulationHashing: public TabulationHashingBase<key_bits, hash_type, chunk_type> {
//   public:
//     typedef TabulationHashingBase<key_bits, hash_type, chunk_type> TB;
//     using TB::table;
//     using TB::table_first_dim;
//     using TB::table_second_dim;

//     hash_type hash( const key_type& key ) const {
//       hash_type ret = 0;
//       key_type tmp = key;
//       for(int i = 0; i < table_first_dim; ++i) {
//         chunk_type curr_chunk = (chunk_type) tmp;
//         ret ^= table[i][curr_chunk];
//         tmp = tmp >> (8*sizeof(chunk_type));
//       }
//       return ret;
//     }

// };

// template <int key_bits, typename hash_type, typename chunk_type>
// class TabulationHashing<std::string, key_bits, hash_type, chunk_type>: 
//     public TabulationHashingBase<key_bits, hash_type, chunk_type> {
//   public:
//     typedef TabulationHashingBase<key_bits, hash_type, chunk_type> TB;
//     using TB::table;
//     using TB::table_first_dim;
//     using TB::table_second_dim;

//     //takes in key, casts each block into a chunk that can then be used as in index
//     // into the table of random numbers
//     hash_type hash( const std::string& key ) const {
//       return hash(key.c_str());
//     }

//     hash_type hash( const char* key) const {
//       hash_type ret = 0;
//       for(int i = 0; i < table_first_dim; ++i) {
//         ret ^= table[i][((chunk_type*) key)[i]];
//       }
//       return ret;
//     }
// };



#endif