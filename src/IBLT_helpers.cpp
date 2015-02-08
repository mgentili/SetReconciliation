#include "IBLT_helpers.hpp"


size_t load_buffer_with_file(const std::string& filename, std::vector<char>& buffer) {
	size_t size = get_file_size(filename);

	FILE* fp = fopen(filename.c_str(), "r");
	if( !fp ) {
		std::cerr << "Unable to open file " << filename << std::endl;
		exit(1);
	}
	buffer.resize(size);
	fread(buffer.data(), 1, size, fp);
	fclose(fp);
	return size;
}

size_t get_file_size(const std::string& filename) {
	FILE* fp1 = fopen(filename.c_str(), "r");
	fseek(fp1, 0, SEEK_END);
	size_t file_size = ftell(fp1);
	fclose(fp1);
	return file_size;
}

std::string get_SHAHash(const std::string& filename) {
	std::vector<char> buf;
	load_buffer_with_file(filename, buf);
	return HashUtil::SHA1Hash( buf.data(), buf.size());
}
// generate_random_file creates a file with len alphanumeric characters
void generate_random_file(const std::string& filename, size_t len) {
	FILE* fp = fopen(filename.c_str(), "w");
	if( !fp ) {
		std::cout << "failed to open file" << std::endl;
		exit(1);
	}
	keyGenerator<std::string, 8> kg; 
	for(size_t i = 0; i < len; ++i) {
		fputc(kg.generate_key()[0], fp);
	}
	fclose(fp);
}

// generate_similar_file creates a file that has on average pct_similarity characters the same 
// and in the same order as the old_file
void generate_similar_file(const std::string& old_file, const std::string& new_file, double pct_similarity) {
	FILE* fp1 = fopen(old_file.c_str(), "r");
	FILE* fp2 = fopen(new_file.c_str(), "w");
	size_t sim = (size_t) (pct_similarity * 100000);
	keyGenerator<uint64_t> kg;
	if( !fp1 || !fp2 ) {
		std::cout << "failed to open file" << std::endl;
		exit(1);
	}
	int c;
	while((c = fgetc(fp1)) != EOF) {
		if( (kg.generate_key() % 100000) < sim) {
			fputc(c, fp2);
		} else {
			fputc( (char) (c+1), fp2 );
		}
	}
	fclose(fp1);
	fclose(fp2);
}

void generate_block_changed_file(const std::string& old_file, const std::string& new_file, size_t num_new_blocks, size_t block_size) {
	keyGenerator<uint64_t> kg;
	std::unordered_set<size_t> start_changes;
		
	FILE* fp1 = fopen(old_file.c_str(), "r");
	size_t file_size = get_file_size(old_file);

	while( start_changes.size() != num_new_blocks ) {
		start_changes.insert( kg.generate_key() % (file_size - block_size ) );
	}

			

	FILE* fp2 = fopen(new_file.c_str(), "w");

	if( !fp1 || !fp2 ) {
		std::cout << "failed to open file" << std::endl;
		exit(1);
	}
	int c;
	size_t counter = 0;
	while( counter < file_size ) {
		if( start_changes.find(counter) != start_changes.end() ) {
			for(size_t i = 0; i < block_size; ++i) {
				c = fgetc(fp1);
				fputc((char) (c+1), fp2);
			}
			counter += block_size;
		} else {
			c = fgetc(fp1);
			fputc( c, fp2);
			counter++;
		}
	}
	fclose(fp1);
	fclose(fp2);
}
