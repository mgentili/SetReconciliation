
#include "IBLT_helpers.hpp"
#include "file_sync.hpp"

int main() {
	static const size_t n_parties = 2;
	typedef uint64_t hash_type;
	typedef FileSynchronizer<n_parties, hash_type> fsync_type;
	fsync_type file_sync;
	const char* file1 = "tmp.txt";
	const char* file2 = "tmp2.txt";
	double similarity = 0.999;
	int file_len = 10000;
	generate_random_file(file1, file_len);
	generate_similar_file(file1, file2, similarity);

	fsync_type::Round1Info r1_A, r1_B;
	file_sync.send_IBLT(file1, r1_A);
	file_sync.send_IBLT(file2, r1_B);
	fsync_type::Round2Info r2_B;
	file_sync.receive_IBLT(file2, r1_B, *(r1_A.iblt), r2_B);

	file_sync.reconstruct_file(file1, r1_A, r2_B);
	return 1;
}