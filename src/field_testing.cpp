#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include "basicField.hpp"

void testFieldChar() {
	std::string buffer;
	Field<3, 64> f;
	std::string key1 = "12345678";
	std::string key2 = "23456789";
	printf("Adding key %s\n", key1.c_str());
	f.add(&key1);
	f.print_contents();
	if( f.extract_key(&buffer) ) {
		printf("Successfully extracted key: %s\n", buffer.c_str());
	} else {
		printf("Failed to extract key\n");
	}
	printf("Adding key %s\n", key2.c_str());
	f.add(&key2);
	f.print_contents();
	if( f.extract_key(&buffer) ) {
		printf("Successfully extracted key: %s\n", buffer.c_str());
	} else {
		printf("Failed to extract key\n");
	}
	printf("Removing key %s\n", key1.c_str());
	f.remove(&key1);
	if( f.extract_key(&buffer) ) {
		printf("Successfully extracted key: %s\n", buffer.c_str());
	} else {
		printf("Failed to extract key\n");
	}
}

void testFieldInt() {
	uint64_t buffer = 0;
	Field<3, 8*sizeof(uint64_t)> f;
	uint64_t x = 123;
	printf("Adding key %lu\n", x);	
	f.add(&x);
	f.print_contents();
	if( f.extract_key(&buffer) ) {
		printf("Successfully extracted key: %lu\n", buffer);
	}
}

void testFieldRemove() {
	uint64_t buffer = 0;
	Field<3, 8*sizeof(uint64_t)> f;
	uint64_t x = 123;
	printf("Removing non-existent key %lu\n", x);
	f.remove(&x);
	f.print_contents();
	if( f.extract_key(&buffer) ) {
		printf("Successfully extracted key: %lu\n", buffer);
	}
}

void testFieldMultiAdd() {
	Field<3, 8*sizeof(uint64_t)> f;
	uint64_t x = 3456;
	for(int i = 0; i < 3; ++i) {
		printf("Adding key %lu\n", x);
		f.add(&x);
		f.print_contents();
	}
}

void testBitField() {
	Field<5, 1> f;
	int x = 1;
	for(int i = 0; i < 3; ++i) {
		printf("Removing key %d\n", x);
		f.remove(&x);
		f.print_contents();
	}
}
int main() {
	testFieldChar();
	testFieldInt();
	testFieldRemove();
	testFieldMultiAdd();
	testBitField();
	return 1;
}