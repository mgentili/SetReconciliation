#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include <iostream>
#include <unordered_set>
#include "basicField.hpp"
#include "IBLT_helpers.hpp"

void testFieldChar() {
	std::string buffer;
	Field<3, std::string, 64> f;
	std::string key1 = "12345678";
	std::string key2 = "23456789";
	printf("Adding key %s\n", key1.c_str());
	f.add(key1);
	f.print_contents();
	if( f.can_divide_by(1) ) {
		f.extract_key(buffer);
		printf("Successfully extracted key: %s\n", buffer.c_str());
	} else {
		printf("Failed to extract key\n");
	}
	printf("Adding key %s\n", key2.c_str());
	f.add(key2);
	f.print_contents();
	if( f.can_divide_by(1) ) {
		f.extract_key(buffer);
		printf("Successfully extracted key: %s\n", buffer.c_str());
	} else {
		printf("Failed to extract key\n");
	}
	printf("Removing key %s\n", key1.c_str());
	f.remove(key1);
	if( f.can_divide_by(1) ) {
		f.extract_key(buffer);
		printf("Successfully extracted key: %s\n", buffer.c_str());
	} else {
		printf("Failed to extract key\n");
	}
}

void testFieldString() {
	std::unordered_set<std::string> strs;
	std::string buffer;
	Field<3, std::string, 64> f;
	keyGenerator<std::string, 64> generator;
	for( int i = 0; i < 10; ++i) {
		strs.insert(generator.generate_key());
	}
	for( auto it = strs.begin(); it != strs.end(); ++it ) {
		printf("Adding key: %s\n", (*it).c_str());
		f.add(*it);
		f.print_contents();
		if( f.can_divide_by(1) ) {
			f.extract_key(buffer);
			printf("Successfully extracted key: %s\n", buffer.c_str());
		} else {
			printf("Failed to extract key\n");
		}
	}
	for( auto it = strs.begin(); it != strs.end(); ++it ) {
		std::cout << "Removing key" << (*it) << std::endl;
		f.remove(*it);
		f.print_contents();
		if( f.can_divide_by(1) ) {
			f.extract_key(buffer);
			printf("Successfully extracted key: %s\n", buffer.c_str());
		} else {
			printf("Failed to extract key\n");
		}
	}
}

void testFieldInt() {
	uint64_t buffer = 0;
	Field<3, uint64_t> f;
	uint64_t x = 123;
	printf("Adding key %lu\n", x);	
	f.add(x);
	f.print_contents();
	if( f.can_divide_by(1) ) {
		f.extract_key(buffer);
		printf("Successfully extracted key: %lu\n", buffer);
	}
}

void testFieldRemove() {
	uint64_t buffer = 0;
	Field<3, uint64_t> f;
	uint64_t x = 123;
	printf("Removing non-existent key %lu\n", x);
	f.remove(x);
	f.print_contents();
	if( f.can_divide_by(-1) ) {
		f.extract_key(buffer);
		printf("Successfully extracted key: %lu\n", buffer);
	}
}

void testFieldMultiAdd() {
	std::unordered_set<uint64_t> keys;
	uint64_t buffer = 0;
	Field<3, uint64_t> f;
	keyGenerator<uint64_t> generator;
	for( int i = 0; i < 10; ++i) {
		keys.insert(generator.generate_key());
	}
	for( auto it = keys.begin(); it != keys.end(); ++it ) {
		printf("Adding key: %lu\n", *it);
		f.add(*it);
		f.print_contents();
		if( f.can_divide_by(1) ) {
			f.extract_key(buffer);
			printf("Successfully extracted key: %lu\n", buffer);
			buffer = 0;
		} else {
			printf("Failed to extract key\n");
		}
	}
	for( auto it = keys.begin(); it != keys.end(); ++it ) {
		std::cout << "Removing key" << *it << std::endl;
		f.remove(*it);
		f.print_contents();
		if( f.can_divide_by(1) ) {
			f.extract_key(buffer);
			printf("Successfully extracted key: %lu\n", buffer);
			buffer = 0;
		} else {
			printf("Failed to extract key\n");
		}
	}
}

void testSimpleField() {
	SimpleField<121> f;
	int x = 25;
	for(int i = 0; i < 10; ++i) {
		printf("Adding key %d\n", x);
		f.add(x);
		f.print_contents();
	}
	for(int i = 0; i < 10; ++i) {
		printf("Removing key %d\n", x);
		f.remove(x);
		f.print_contents();
	}
	assert(f.get_contents() == 0);
	for(int i = 0; i < 10; ++i) {
		printf("Removing key %d\n", x);
		f.remove(x);
		f.print_contents();
	}
	for(int i = 0; i < 10; ++i) {
		printf("Adding key %d\n", x);
		f.add(x);
		f.print_contents();
	}
	assert(f.get_contents() == 0);
}

void testFieldSum() {
	Field<3, uint64_t> f1, f2;
	uint64_t x = 3456;
	uint64_t y = 3456;
	f1.add(x);
	f2.add(y);
	f1.add(f2);
	f1.print_contents();
	f1.remove(f2);
	f1.print_contents();
}
int main() {
	testFieldChar();
	testFieldInt();
	testFieldRemove();
	testFieldString();
	testSimpleField();
	testFieldSum();
	testFieldMultiAdd();
	
	return 1;
}