#include "fingerprinting.hpp"
#include "IBLT_helpers.hpp"
#include <stdio.h>

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

template <typename hash_type = uint64_t>
class FingerprintTester {
  public:
	size_t kgrams;
	size_t window_size;
	Fingerprinter<hash_type> f;
	keyHandler<hash_type> kh;

	FingerprintTester(size_t kgrams, size_t window_size): 
		kgrams(kgrams), window_size(window_size), f(kgrams, window_size) {}

	void basicTest(const char* filename) {
		std::vector<pair<hash_type, int> > hashes;
		size_t file_size = f.get_fingerprint(filename, hashes);
		// for(size_t i = 0; i < hashes.size(); ++i) {
		// 	std::cout << "Hash: " << hashes[i].first << ", Pos: " << hashes[i].second << std::endl;
		// }
		std::cout << "Density is " << (double) hashes.size()/ file_size << "compared to theoretical " << 2.0/(window_size+1) << std::endl; 
	}

	// filters out just the first component of the pair<hash_type, int>
	void filterHashes( std::vector<pair<hash_type, int> >& hashes, std::unordered_set<hash_type>& hash_set) {
		for(auto it = hashes.begin(); it != hashes.end(); ++it) {
			hash_set.insert(it->first);
		}
	}

	// computes the jaccard coefficient of two sets
	void comparisonTest(const char* f1, const char* f2) {
		std::vector<pair<hash_type, int> > hashes1, hashes2;
		std::unordered_set<hash_type> hash_set1, hash_set2, hash_union, hash_intersection;
		f.get_fingerprint(f1, hashes1);
		f.get_fingerprint(f2, hashes2);
		filterHashes(hashes1, hash_set1);
		filterHashes(hashes2, hash_set2);
		// for(size_t i = 0; i < hashes2.size(); ++i) {
		// 	std::cout << "Hash: " << hashes2[i].first << ", Pos: " << hashes2[i].second << std::endl;
		// }
		kh.set_union(hash_set1, hash_set2, hash_union);
		/*for(auto it = hash_set2.begin(); it != hash_set2.end(); ++it) {
			std::cout << "Hash: " << *it << std::endl;
		}*/
		kh.set_intersection(hash_set1, hash_set2, hash_intersection);
		std::cout << "File1 hash size " << hashes1.size() << "File2 hash size " << hashes2.size() << std::endl;
		std::cout << "Intersection is " << hash_intersection.size() << "Union is " << hash_union.size() << std::endl;
		std::cout << "Similarity is" << (double) hash_intersection.size()/ hash_union.size() << std::endl; 
	}

	void fileDigest(const char* f1) {
		std::vector<pair<hash_type, int> > hashes;
		f.digest_file(f1, hashes);
		// for(size_t i = 0; i < hashes.size(); ++i) {
		// 	std::cout << "Hash: " << hashes[i].first << ", Len: " << hashes[i].second << std::endl;
		// }
	}

	void fileDigestComparison(const char* f1, const char* f2) {
		std::vector<pair<hash_type, int> > hashes1, hashes2;
		std::unordered_set<hash_type> hash_set1, hash_set2, hash_union, hash_intersection;
		f.digest_file(f1, hashes1);
		f.digest_file(f2, hashes2);
		filterHashes(hashes1, hash_set1);
		filterHashes(hashes2, hash_set2);
		kh.set_union(hash_set1, hash_set2, hash_union);
		kh.set_intersection(hash_set1, hash_set2, hash_intersection);
		std::cout << "File1 hash size " << hashes1.size() << "File2 hash size " << hashes2.size() << std::endl;
		std::cout << "Intersection is " << hash_intersection.size() << "Union is " << hash_union.size() << std::endl;
		std::cout << "Similarity is" << (double) hash_intersection.size()/ hash_union.size() << std::endl; 
	}
};


int main() {
	const size_t kgrams = 8;
	const size_t window_size = 100;
	typedef uint64_t hash_type;
	FingerprintTester<hash_type> ft( kgrams, window_size);
	const char* file1 = "tmp.txt";
	const char* file2 = "tmp2.txt";
	double similarity = 0.999;
	int file_len = 10000;
	generate_random_file(file1, file_len);
	ft.basicTest(file1);
	generate_similar_file(file1, file2, similarity);
	//ft.basicTest(file2);
	ft.comparisonTest(file1, file2);
	ft.fileDigest(file1);
	ft.fileDigestComparison(file1, file2);
	return 1;
}