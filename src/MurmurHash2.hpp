#ifndef _MURMUR
#define _MURMUR

#include <stdint.h>
#include <vector>
#include <random>
#include <assert.h>


//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby

// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
// and endian-ness issues if used across multiple platforms.

// 64-bit hash for 64-bit platforms

uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed );

template <typename key_type, size_t key_bits, typename hash_type>
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

	hash_type hash( const key_type& k) {
		return MurmurHash64A( &k, key_bits/8, seed );
	}
};



#endif