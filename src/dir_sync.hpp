#ifndef _DIR_SYNC
#define _DIR_SYNC

#include "boost/filesystem.hpp"
#include <vector>
#include <string>
#include "hash_util.hpp"
#include "multiIBLT.hpp"
#include "IBLT_helpers.hpp"
#include "StrataEstimator.hpp"

class dir_sync {
  public:
  	std::vector<std::string> filenames, content_hashes, new_files;
  	std::unordered_set<std::string> file_hashes, keys;
  	std::unordered_map<std::string, std::string> contenthash_to_file, filehash_to_file;
  	std::string path;
  	static const size_t n_parties = 2;
  	typedef uint32_t hash_type;
  	typedef multiIBLT<n_parties, std::string, 40*8> iblt_type;
  	typedef StrataEstimator<hash_type> estimator_type;
  	iblt_type* iblt;
  	estimator_type estimator;

  	// info updatee needs to rename files and delete old ones
  	class update_info {
  	  public:
  		std::vector<std::string> old_file_hashes;
  		std::unordered_map<std::string, std::string> filehash_to_new_file;
  	};

  	dir_sync(std::string p): path(p), iblt(NULL) {}
  	~dir_sync() {
  		delete iblt;
  	}
  	//TODO: Figure out how to do relative path more efficiently
  	void get_files() {
  		for ( boost::filesystem::recursive_directory_iterator end, dir(path);
		    dir != end; ++dir ) {
  			std::string full_path = dir->path().string();
  			std::cout << full_path << std::endl;
			std::string relative_path = get_relative_path(full_path);
  			if( is_directory(dir->path()) ) { //TODO: Handle directories
  				std::cout << relative_path << " is a directory" << std::endl;
  			} else if( relative_path[0] == '.' ) {
				std::cout << relative_path << " is a hidden file" << std::endl;
			} else {
  				filenames.push_back(relative_path);
  			}
		}
  	}

  	//based on my directory, create all relevant data structures
  	void process_dir() {
  		get_files();
  		for(auto it = filenames.begin(); it != filenames.end(); ++it) {
  			std::string new_file_hash = get_file_hash(*it);
  			file_hashes.insert(new_file_hash);
			if( filehash_to_file.find(new_file_hash) != filehash_to_file.end() )
				std::cout << "Filehash: " << new_file_hash << " corresponds to "
					  << filehash_to_file[new_file_hash] << " and " 
					  << *it << std::endl;
  			filehash_to_file[new_file_hash] = *it;

  			std::string new_content_hash = get_content_hash(*it);
  			content_hashes.push_back(new_content_hash);
  			if( contenthash_to_file.find(new_content_hash) != contenthash_to_file.end() )
				std::cout << "Contenthash: " << new_content_hash << " corresponds to "
					  << contenthash_to_file[new_content_hash] << " and " 
					  << *it << std::endl;
			contenthash_to_file[new_content_hash] = *it;

  			std::string new_key = get_file_content_key(new_file_hash, new_content_hash);
  			keys.insert(new_key);
  		}
  		assert(filenames.size() == filehash_to_file.size()); //filehashes need to be unique
  		assert(filenames.size() == contenthash_to_file.size()); //TODO what if contents the same?
  		createEstimator();
  	}

  	static std::string get_file_hash(std::string& filename) {
  			return HashUtil::SHA1Hash(filename.c_str(), filename.size());
  	}

  	std::string get_absolute_path(std::string& relative_path) {
  		return path + "/" + relative_path;
  	}

  	std::string get_relative_path(std::string& absolute_path) {
  		return absolute_path.substr(path.size() + 1, absolute_path.size() - path.size() - 1);
  	}

  	std::string get_content_hash(std::string& filename) {
		char* buf;
		size_t filesize = load_buffer_with_file(get_absolute_path(filename).c_str(), &buf);
		std::string content_hash = HashUtil::SHA1Hash(buf, filesize);
		delete[] buf;
		return content_hash;
  	}

  	static std::string get_file_content_key(std::string& file_hash, std::string& content_hash) {
  		return file_hash + content_hash;
  	}

  	void createEstimator() {
  		for(auto it = keys.begin(); it != keys.end(); ++it) {
  			estimator.insert_key(*it);
  		}
  	}

  	std::string get_file_hash(const std::string& file_contents) {
  		return file_contents.substr(0, 20);
  	}

  	std::string get_content_hash(const std::string& file_contents) {
  		return file_contents.substr(20, 20);
  	}

  	size_t set_difference_estimate(estimator_type& cp_estimator) {
  		return(estimator.estimate_diff(cp_estimator));
  	}

  	//based on bucket estimate, create an IBLT
  	void create_IBLT(size_t bucket_estimate) {
  		size_t num_buckets = bucket_estimate * 2;
  		size_t num_hashfns = ( bucket_estimate < 200 ) ? 3 : 4;
  		iblt = new iblt_type(num_buckets, num_hashfns);
  		iblt->insert_keys(keys);
  	}

  	//based on my IBLT and counterparty IBLT, determine which files are shared/not shared
  	//update_info find_differences(dir_sync& cp_IBLT) {
  	update_info find_differences(iblt_type& cp_IBLT) {
  		update_info new_info;
  		iblt_type resIBLT(cp_IBLT.num_buckets, cp_IBLT.num_hashfns);
  		
  		resIBLT.add(*iblt, 0); //my keys
  		resIBLT.add(cp_IBLT, 1); //counterparty keys

  		std::unordered_map<std::string, std::vector<int> > distinct_keys;
  		bool res = resIBLT.peel(distinct_keys);
  		if( !res ) {
  			std::cout << "Failed to peel, need to retry" << std::endl;
  			exit(1);
  		}

  		std::cout << "Distict keys has size " << distinct_keys.size();
  		
  		int counter=  0;
  		//for files that share contents but not name, just send file -> new file name map
  		for(auto it = distinct_keys.begin(); it != distinct_keys.end(); ++it) {
  			if( keys.find(it->first) == keys.end()) {//counterparty has
  				counter++;
	  			std::string content_hash = get_content_hash(it->first); 
	  			std::string file_hash = get_file_hash(it->first);
  				if( contenthash_to_file.find(content_hash) != contenthash_to_file.end()) {
  					std::string filename = contenthash_to_file[content_hash];
  					new_info.filehash_to_new_file[file_hash] = filename;
  				} else { //contenthash doesn't exist, meaning that the file was deleted on this machine
  					new_info.old_file_hashes.push_back(file_hash);
  				}
  			} else {
  				std::cout << "I have " << filehash_to_file[get_file_hash(it->first)] << "and counterparty doesn't" << std::endl;
  			}
  		}
  		std::cout << "Counterparty has this many distinct keys: " << counter << std::endl;
  		std::cout << "Filehash map size: " << filehash_to_file.size() << std::endl;
  		std::cout << "Contenthash map size: " << contenthash_to_file.size() << std::endl;

  		for(auto it = contenthash_to_file.begin(); it != contenthash_to_file.end(); ++it) {
  			std::cout << it->second << std::endl;
  		}
  		
  		//for entirely new files, send both new file name and new file contents (using rsync)

  		//for now non-existent files, just send file name	

  		return new_info;
  	}

  	void process_differences(update_info& new_info) {
  		std::cout << "Total number of files needed to be renamed" << new_info.filehash_to_new_file.size() << std::endl;
  		for(auto it = new_info.filehash_to_new_file.begin(); it != new_info.filehash_to_new_file.end(); ++it) {
  			assert(filehash_to_file.find(it->first) != filehash_to_file.end());
  			std::string old_file_path = get_absolute_path(filehash_to_file[it->first]);

  			std::cout << "File hash " << " corresponding to " << 
  			filehash_to_file[it->first] << "needs to become new file " << it->second << std::endl;
  			boost::filesystem::path old_path(old_file_path), new_path(get_absolute_path(it->second));
  			boost::filesystem::rename(old_path, new_path);
  		}

  		for(auto it = new_info.old_file_hashes.begin(); it != new_info.old_file_hashes.end(); ++it) {
  			std::cout << "File hash " << *it << " corresponding to " << 
  			filehash_to_file[*it] << "needs to be deleted" << std::endl;

  		}
  	}

};

#endif
