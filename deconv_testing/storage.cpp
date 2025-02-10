#include "storage.hpp"

namespace Storage {

	std::map<const std::string, Image> images;
	std::map<const std::string, FileLike> filelikes;
	
	std::map<const std::string, std::list<std::function<void()>>> deconv_task_buffers;

	std::map<const std::string, std::vector<std::byte>> named_blobs;
	
	const std::string get_opened_file_name(const std::string& file_name){
		return file_name + OPEN_FILE_TAG;
	}
}

