#include "IBLT_helpers.hpp"

size_t load_buffer_with_file(const char* filename, char** buf) {
	FILE* fp = fopen(filename, "r");
	if( !fp ) {
		std::cerr << "Unable to open file" << std::endl;
		exit(1);
	}

	struct stat st;
	stat(filename, &st);
	size_t size = st.st_size;
	*buf = new char[size];
	fread(*buf, 1, size, fp);
	fclose(fp);
	return size;
}

// generate_random_file creates a file with len alphanumeric characters
void generate_random_file(const char* filename, size_t len) {
	FILE* fp = fopen(filename, "w");
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
void generate_similar_file(const char* old_file, const char* new_file, double pct_similarity) {
	FILE* fp1 = fopen(old_file, "r");
	FILE* fp2 = fopen(new_file, "w");
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
		}
	}
	fclose(fp1);
	fclose(fp2);
}
