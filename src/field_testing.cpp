#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include <iostream>
#include <unordered_set>
#include "basicField.hpp"
#include "IBLT_helpers.hpp"
#include "hash_util.hpp"

void testFieldChar() {
	std::string buffer;
	Field<3, std::string, 64> f;
	std::string key1 = "12345678";
	std::string key2 = "23456789";
	std::cout << "Adding key: " << key1 << std::endl;
	f.add(key1);
	f.print_contents();
	assert( f.can_divide_by(1) );
	if( f.can_divide_by(1) ) {
		f.extract_key(buffer);
		std::cout << "Successfully extracted key: " << buffer << std::endl;
		assert(key1 == buffer);
	} else {
		std::cout << "Failed to extract key" << std::endl;
	}
	std::cout << "Adding key: " << key2 << std::endl;
	f.add(key2);
	f.print_contents();
	if( f.can_divide_by(1) ) {
		f.extract_key(buffer);
		std::cout << "Successfully extracted key: " << buffer << std::endl;
		assert(key1 == buffer);
	} else {
		std::cout << "Failed to extract key" << std::endl;
	}
	std::cout << "Removing key: " << key1 << std::endl;
	f.remove(key1);
	assert( f.can_divide_by(1) );
	if( f.can_divide_by(1) ) {
		f.extract_key(buffer);
		std::cout << "Successfully extracted key: " << buffer << std::endl;
		assert(key2 == buffer);
	} else {
		std::cout << "Failed to extract key" << std::endl;
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
		std::cout << "Adding key: " << (*it) << std::endl;
		f.add(*it);
		f.print_contents();
		if( f.can_divide_by(1) ) {
			f.extract_key(buffer);
			std::cout << "Successfully extracted key: " << buffer << std::endl;
			assert(*it == buffer);
		} else {
			std::cout << "Failed to extract key" << std::endl;
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

void test2FieldInt() {
	Field<2, uint64_t> f1;
	uint64_t x1 = 1;
	uint64_t x2 = 1343415;
	printf("Adding key: %lu\n", x1);
	f1.add(x1);
	f1.print_contents();
	printf("Adding key: %lu\n", x2);
	f1.add(x2);
	f1.print_contents();
}

void test2FieldString() {
	Field<2, std::string, 64> f1;
	std::string buf;
	std::string key1 = "12345678";
	printf("Adding key: %s\n", key1.c_str());
	f1.add(key1);
	f1.print_contents();
	f1.add(key1);
	f1.print_contents();

	Field<2, std::string, 320> f2;
	key1 = "1234567890123456789012345678901234567890";
	printf("Adding key: %s\n", key1.c_str());
	f2.add(key1);
	f2.print_contents();
	f2.add(key1);
	f2.print_contents();
	printf("Extracted key: %s\n", buf.c_str());

	std::string shad = HashUtil::SHA1Hash(key1.c_str(), key1.size());
	shad = shad + shad;
	std::cout << "Adding key: " << shad << " with length " << shad.size() << std::endl;
	f2.add(shad);
	f2.print_contents();
	std::string extracted;
	f2.extract_key(extracted);
	f2.add(shad);
	f2.print_contents();
	f2.add(extracted);
	f2.print_contents();
	std::cout << "Extracted key: " << extracted << " with size " << extracted.size() << std::endl;

}

void testMultiply() {
	Field<17, uint32_t> f1;
	uint32_t x = 245246345;
	int k = 13;
	f1.add(x);
	f1.print_contents();
	f1.add(x);
	f1.print_contents();
	f1.multiply(k);
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
	test2FieldInt();
	test2FieldString();
	testMultiply();
	return 1;
}
