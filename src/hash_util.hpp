#ifndef _HASHUTIL_H_
#define _HASHUTIL_H_

#include <sys/types.h>
#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <openssl/evp.h>
#include <assert.h>
#include <iostream>

class HashUtil {
public:
    // Bob Jenkins Hash
    static uint32_t BobHash(const void *buf, size_t length, uint32_t seed = 0);
    static uint32_t BobHash(const std::string &s, uint32_t seed = 0);

    // Bob Jenkins Hash that returns two indices in one call
    // Useful for Cuckoo hashing, power of two choices, etc.
    // Use idx1 before idx2, when possible. idx1 and idx2 should be initialized to seeds.
    static void BobHash(const void *buf, size_t length, uint32_t *idx1,  uint32_t *idx2);
    static void BobHash(const std::string &s, uint32_t *idx1,  uint32_t *idx2);

    // MurmurHash2
    static uint32_t MurmurHash(const void *buf, size_t length, uint32_t seed = 0);
    static uint32_t MurmurHash(const std::string &s, uint32_t seed = 0);
    static uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed );

    // SuperFastHash
    static uint32_t SuperFastHash(const void *buf, size_t len);
    static uint32_t SuperFastHash(const std::string &s);

    // Null hash (shift and mask)
    static uint32_t NullHash(const void* buf, size_t length, uint32_t shiftbytes);

    // Wrappers for MD5 and SHA1 hashing using EVP
    static std::string MD5Hash(const char* inbuf, size_t in_length);
    static std::string SHA1Hash(const char* inbuf, size_t in_length);

private:
    HashUtil();
};

template <size_t key_bits, typename hash_type>
class MurmurHashing {
  public:
    uint64_t seed;
    static const unsigned int num_seeds = 16;
    const uint64_t predef_seeds[num_seeds] =  {
        0xC3DA4A8C, 0xA5112C8C, 0x5271F491, 0x9A948DAB,
        0xCEE59A8D, 0xB5F525AB, 0x59D13217, 0x24E7C331,
        0x697C2103, 0x84B0A460, 0x86156DA9, 0xAEF2AC68,
        0x23243DA5, 0x3F649643, 0x5FA495A8, 0x67710DF8
    };

    MurmurHashing(): seed(0) {}
    MurmurHashing(uint64_t s): seed(s) {}
    
    void set_seed(uint64_t s) {
        assert( s < num_seeds );
        seed = predef_seeds[s];
    }
    hash_type hash( const std::string& k) {
        return HashUtil::MurmurHash64A( k.c_str(), key_bits/8, seed );
    }

    hash_type hash( const uint64_t& k) {
        return HashUtil::MurmurHash64A( &k, key_bits/8, seed );
    }
};



#endif  // #ifndef _HASHUTIL_H_