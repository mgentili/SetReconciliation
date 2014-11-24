#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include "basicField.hpp"

void testFieldChar() {
	char buffer[100] = {};
	Field<3, 64> f;
	const char* key1 = "12345678";
	const char* key2 = "23456789";
	printf("Adding key %s\n", key1);
	f.add(key1);
	f.print_contents();
	if( f.extract_key(buffer) ) {
		printf("Successfully extracted key: %s\n", buffer);
	} else {
		printf("Failed to extract key\n");
	}
	printf("Adding key %s\n", key2);
	f.add(key2);
	f.print_contents();
	memset(buffer, 0, 100);
	if( f.extract_key(buffer) ) {
		printf("Successfully extracted key: %s\n", buffer);
	} else {
		printf("Failed to extract key\n");
	}
	printf("Removing key %s\n", key1);
	f.remove(key1);
	memset(buffer, 0, 100);
	if( f.extract_key(buffer) ) {
		printf("Successfully extracted key: %s\n", buffer);
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
int main() {
	testFieldChar();
	testFieldInt();
	testFieldRemove();
	testFieldMultiAdd();
	return 1;
}