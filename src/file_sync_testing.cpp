#include "IBLT_helpers.hpp"
#include "file_sync.hpp"
#include "file_sync.pb.h"
#include "boost/filesystem.hpp"

void testFullProtocol(const char* file1, const char* file2) {
	typedef uint32_t hash_type;
	typedef FileSynchronizer<hash_type> fsync_type;

	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
	//Determine estimated file difference
	fsync_type file_sync_A(file1), file_sync_B(file2);
	std::string strata_encoding = file_sync_A.send_strata_encoding();
	size_t diff_est = file_sync_B.receive_strata_encoding(strata_encoding);
	std::cout << "Difference estimate is: " << diff_est << " meaning IBLT is of size "
			  << diff_est*2 << std::endl;
	std::string iblt_encoding = file_sync_A.send_IBLT_encoding(diff_est);
	std::string rd2_encoding = file_sync_B.receive_IBLT_encoding(iblt_encoding);
	file_sync_A.receive_rd2_encoding(rd2_encoding);
	file_sync_B.my_rd2.print_size_info();
	size_t total_bytes_no_strata = iblt_encoding.size() + rd2_encoding.size();
	size_t total_bytes = total_bytes_no_strata + strata_encoding.size();
	std::cout << "Total number of bytes transferred: " << total_bytes << std::endl;
	std::cout << "Bytes transferred without strata : " << total_bytes_no_strata << std::endl;

	size_t file1_size = boost::filesystem::file_size(file1);
	size_t file2_size = boost::filesystem::file_size(file2); 
	std::cout << "File size 1: " << file1_size << std::endl
			  << "File size 2: " << file2_size << std::endl;

	char* buf;
	load_buffer_with_file(file1, &buf);
	std::string file1_string(buf, file1_size);
	delete[] buf;
	load_buffer_with_file(file2, &buf);
	std::string file2_string(buf, file2_size);
	delete[] buf;
	std::string file1_compressed = compress_string(file1_string);
	std::string file2_compressed = compress_string(file2_string);

	std::cout << "File size 1 compressed: " << file1_compressed.size() << std::endl
			  << "File size 2 compressed: " << file2_compressed.size() << std::endl;
}


int main(int argc, char* argv[]) {
	if( argc < 3 ) {
		std::cout << "./bin/file_sync_testing [file1] [file2] ([length] [similarity])" << std::endl;
		exit(1);
	}
	const char* file1 = argv[1];
	const char* file2 = argv[2];

	if(argc == 5) {
		int file_len = stoi(argv[3]);
		double similarity = std::stod(argv[4]);
		std::cout << "Similarity: " << similarity << ", File length: " << file_len << ", file1: " << file1 << ", file2: " << file2 << std::endl;
		generate_random_file(file1, file_len);
		generate_similar_file(file1, file2, similarity);
	}

	testFullProtocol(file1, file2);
	return 1;
}