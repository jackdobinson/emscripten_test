#ifndef __STORAGE_INCLUDED__
#define __STORAGE_INCLUDED__

#include <map>
#include <list>
#include <functional>
#include <vector>
#include <span>
#include "image.hpp"
#include "file_like.hpp"
#include "tiffio.h"


namespace Storage{
	template<class T> 
	std::map<const std::string, T> objects;

	extern std::map<const std::string, Image> images;
	extern std::map<const std::string, FileLike> filelikes;
	
	extern std::map<const std::string, std::list<std::function<void()>>> deconv_task_buffers;
	
	
	extern std::map<const std::string, std::vector<std::byte>> named_blobs; // store named blobs here
	
	const std::string OPEN_FILE_TAG="__open_copy";
	const std::string get_opened_file_name(const std::string& file_name);
}



#endif //__STORAGE_INCLUDED__
