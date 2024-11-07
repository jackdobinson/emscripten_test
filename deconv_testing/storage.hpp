#ifndef __STORAGE_INCLUDED__
#define __STORAGE_INCLUDED__

#include <vector>
#include <span>
#include "image.hpp"
#include "file_like.hpp"
#include "tiffio.h"


namespace Storage{

	using blob_id_t = size_t;

	extern std::map<std::string, Image> images;
	extern std::map<std::string, FileLike> filelikes;
	extern std::map<blob_id_t, std::vector<std::byte>> blobs; // binary representation of stuff goes here.

	namespace BlobMgr{
		extern blob_id_t last_id_given;

		blob_id_t get_free_id();

		template<class T=uint8_t>
		std::span<T> get_as(blob_id_t blob_id){
			const std::vector<std::byte>& item = blobs[blob_id];
			return std::span<T>((T*)item.data(), (T*)(item.data()+item.size()));
		}
	}
}

enum class FileType {
	TIFF
};

struct FPtr{
	FileType ft;
	void* p;
};
// Should make this into a class
extern std::map<const std::string, FPtr> MemFileRegistry;

int MemFileRegistry_delete(const std::string& key);

void MemFileRegistry_destroy();

template<class T>
T* MemFileRegistry_get(const std::string& key){
	return (T*)(MemFileRegistry[key].p);
}


#endif //__STORAGE_INCLUDED__
