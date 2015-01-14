#include "IBLT_helpers.hpp"
#include "file_sync.hpp"
#include "file_sync.pb.h"
#include <zlib.h>

#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>

/** Compress a STL string using zlib with given compression level and return
  * the binary data. */
std::string compress_string(const std::string& str,
                            int compressionlevel = Z_BEST_COMPRESSION)
{
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, compressionlevel) != Z_OK)
        throw(std::runtime_error("deflateInit failed while compressing."));

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();           // set the z_stream's input

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            // append the block to the output string
            outstring.append(outbuffer,
                             zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return outstring;
}

/** Decompress an STL string using zlib and return the original data. */
std::string decompress_string(const std::string& str)
{
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (inflateInit(&zs) != Z_OK)
        throw(std::runtime_error("inflateInit failed while decompressing."));

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // get the decompressed bytes blockwise using repeated calls to inflate
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = inflate(&zs, 0);

        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer,
                             zs.total_out - outstring.size());
        }

    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib decompression: (" << ret << ") "
            << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return outstring;
}

void testFullProtocol(const char* file1, const char* file2) {
	static const size_t n_parties = 2;
	typedef uint32_t hash_type;
	typedef FileSynchronizer<n_parties, hash_type> fsync_type;
	fsync_type file_sync_A, file_sync_B;

	GOOGLE_PROTOBUF_VERIFY_VERSION;
	
	//Determine estimated file difference
	file_sync_A.process_file(file1);
	file_sync_B.process_file(file2);
	size_t diff_est = file_sync_B.get_difference_estimate(file_sync_A.my_rd1.estimator);
	std::cout << "File1 Hashes: " << file_sync_A.my_rd1.hashes_to_poslen.size() << "File2 Hashes: " << file_sync_B.my_rd1.hashes_to_poslen.size() << std::endl;
	std::cout << "Difference estimate is " << diff_est << std::endl;
	
	//Create IBLTs based on estimated difference, and process to determine differences
	file_sync_A.create_IBLT(diff_est);
	file_sync_B.create_IBLT(diff_est);

	std::unordered_set<hash_type> distinct_keys;
	for(auto it = file_sync_A.my_rd1.hashes_to_poslen.begin(); it != file_sync_A.my_rd1.hashes_to_poslen.end(); ++it) {
		if( file_sync_B.my_rd1.hashes_to_poslen.find(it->first) == file_sync_B.my_rd1.hashes_to_poslen.end()) {
			distinct_keys.insert(it->first);
		}
	}

	for(auto it = file_sync_B.my_rd1.hashes_to_poslen.begin(); it != file_sync_B.my_rd1.hashes_to_poslen.end(); ++it) {
		if( file_sync_A.my_rd1.hashes_to_poslen.find(it->first) == file_sync_A.my_rd1.hashes_to_poslen.end()) {
			distinct_keys.insert(it->first);
		}
	}
	std::cout << "Actual number of distinct keys is" << distinct_keys.size() << std::endl;
	//Party A sends his IBLT to Party B
	// file_sync::IBLT A_iblt_protobuf, iblt_recreated;
	// (file_sync_A.my_rd1.iblt)->serialize(A_iblt_protobuf);
	// std::string A_iblt_bits;
	// A_iblt_protobuf.SerializeToString(&A_iblt_bits);
	// std::cout << "Serialized iblt structure is size (bits) " << A_iblt_bits.size()*8 << "vs actual" << (file_sync_A.my_rd1.iblt)->size_in_bits() << std::endl;
	// A_iblt_bits = compress_string(A_iblt_bits);
	// std::cout << "After compression Serialized iblt structure is size (bits) " << A_iblt_bits.size()*8 << std::endl;
	// A_iblt_bits = decompress_string(A_iblt_bits);
	// iblt_recreated.ParseFromString(A_iblt_bits);
	// fsync_type::iblt_type new_iblt(file_sync_B.my_rd1.iblt->num_buckets, file_sync_B.my_rd1.iblt->num_hashfns);
	// new_iblt.deserialize(iblt_recreated);

	//Party B receives IBLT and 
	//file_sync_B.receive_IBLT(file2, new_iblt);
	// file_sync_B.receive_IBLT(file2, *(file_sync_A.my_rd1.iblt));
	file_sync_B.receive_IBLT(file2, *(file_sync_A.my_rd1.iblt), file_sync_A.my_rd1);


	// file_sync::Round2 rd2_serialized;
	// file_sync_B.my_rd2.serialize(rd2_serialized);
	// file_sync::Round2 rd2_B_recreated;
	// std::string rd2_B_bits;
	// rd2_serialized.SerializeToString(&rd2_B_bits);
	// std::cout << "Serialized structure is size (bits) " << rd2_B_bits.size()*8 << "vs actual" << file_sync_B.my_rd2.size_in_bits() << std::endl;
	// rd2_B_bits = compress_string(rd2_B_bits);
	// std::cout << "After compression Serialized structure is size (bits) " << rd2_B_bits.size()*8 << std::endl;
	// rd2_B_bits = decompress_string(rd2_B_bits);
	// rd2_B_recreated.ParseFromString(rd2_B_bits);

	// fsync_type::Round2Info rd2_B_2;
	// rd2_B_2.deserialize(rd2_B_recreated);
	// file_sync_A.reconstruct_file(file1, rd2_B_2);
	file_sync_A.reconstruct_file(file1, file_sync_B.my_rd2);

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
	std::string a = "a";
	std::string b = "b";
	std::cout << HashUtil::MurmurHash64A ( a.c_str(), 1, 0 ) << std::endl;
	std::cout << HashUtil::MurmurHash64A ( b.c_str(), 1, 0 ) << std::endl;

	return 1;
}