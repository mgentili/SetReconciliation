#include <stdint.h>

template <typename key_type>
class MurmurWrapper {
  public:
	uint64_t seed;
	static const unsigned int num_seeds = 16;
	const uint64_t predef_seeds[num_seeds] =  {
		0xC3DA4A8C, 0xA5112C8C, 0x5271F491, 0x9A948DAB,
        0xCEE59A8D, 0xB5F525AB, 0x59D13217, 0x24E7C331,
        0x697C2103, 0x84B0A460, 0x86156DA9, 0xAEF2AC68,
        0x23243DA5, 0x3F649643, 0x5FA495A8, 0x67710DF8
	};

	MurmurWrapper(): seed(0) {}
	MurmurWrapper(uint64_t s): seed(s) {}
	void set_seed(uint64_t s) {
		seed = s;
	}
	void set_seed_ind(uint64_t s) {
		assert( s < num_seeds );
		seed = predef_seeds[s];
	}

	uint64_t hash( key_type k) {
		return MurmurHash64A( &k, sizeof(key_type)/8, seed );
	}
};


//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby

// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
// and endian-ness issues if used across multiple platforms.

// 64-bit hash for 64-bit platforms

uint64_t MurmurHash64A ( const void * key, int len, unsigned int seed )
{
	const uint64_t m = 0xc6a4a7935bd1e995;
	const int r = 47;

	uint64_t h = seed ^ (len * m);

	const uint64_t * data = (const uint64_t *)key;
	const uint64_t * end = data + (len/8);

	while(data != end)
	{
		uint64_t k = *data++;

		k *= m; 
		k ^= k >> r; 
		k *= m; 
		
		h ^= k;
		h *= m; 
	}

	const unsigned char * data2 = (const unsigned char*)data;

	switch(len & 7)
	{
	case 7: h ^= uint64_t(data2[6]) << 48;
	case 6: h ^= uint64_t(data2[5]) << 40;
	case 5: h ^= uint64_t(data2[4]) << 32;
	case 4: h ^= uint64_t(data2[3]) << 24;
	case 3: h ^= uint64_t(data2[2]) << 16;
	case 2: h ^= uint64_t(data2[1]) << 8;
	case 1: h ^= uint64_t(data2[0]);
	        h *= m;
	};
 
	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
} 


//--HASHFUNCTION STUFF

/**
Simple hash function family (a*x+b),
where a and b are randomly seeded odd integers
**/
template <typename key_type>
class simple_hash {
  public:
	uint64_t a;
	uint64_t b;
	uint64_t p;

	static const unsigned int num_seeds = 8;
	const uint64_t predef_p[num_seeds] =  {
		0xEC4BAB, 0x75BCD17, 0x12D691, 0x35CB67,
        0x15036FB3, 0x321CCBF, 0x43D135, 0x54C56F
	};

	const uint64_t predef_a[num_seeds] =  {
		0xC3DA4A8C, 0xA5112C8C, 0x5271F491, 0x9A948DAB,
        0xCEE59A8D, 0xB5F525AB, 0x59D13217, 0x24E7C331,

	};

	const uint64_t predef_b[num_seeds] =  {
        0x697C2103, 0x84B0A460, 0x86156DA9, 0xAEF2AC68,
        0x23243DA5, 0x3F649643, 0x5FA495A8, 0x67710DF8
	};

	simple_hash(): a(0), b(0) {}
	simple_hash( int seed ) {
		set_seed_ind(seed);
	}

	void set_seed_ind( unsigned int seed ) {
		assert( seed < num_seeds );
		a = predef_a[seed];
		b = predef_b[seed];
		p = predef_p[seed];
		//printf("seed is %d, a is %ld, b is %ld\n", seed, a, b);
	}

	key_type hash( uint64_t k ) {
		return (a*k + b) % p;
	}
};
