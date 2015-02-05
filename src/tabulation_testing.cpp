#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include "tabulation_hashing.hpp"

int testString() {
	const int key_bits = 32*8;
	typedef uint16_t chunktype;
	typedef uint64_t hashtype;
	//typedef TabulationHashing<std::string, key_bits, hashtype, chunktype> TB;
	typedef TabulationHashing<key_bits, hashtype, chunktype> TB;
	TB* tb = new TB;
	tb->set_seed(0);
	std::string key = "12345678123456781234567812345678";
	printf("%ld\n", (long) tb->hash(key) );
	delete tb;
	return 1;
}

int testInt() {
	typedef uint64_t key_type;
	typedef uint16_t chunktype;
	typedef uint64_t hashtype;
	//typedef TabulationHashing<key_type, 8*sizeof(key_type), hashtype, chunktype> TB;
	typedef TabulationHashing<8*sizeof(key_type), hashtype, chunktype> TB;
	
	TB* tb = new TB;
	tb->set_seed(0);
	key_type key = 1341235;
	printf("%ld\n", (long) tb->hash(key) );
	delete tb;
	return 1;
}


int main() {
	testString();
	testInt();
	return 1;
}
