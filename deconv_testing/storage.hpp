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
	
	extern std::map<const std::string, blob_id_t> blob_name_to_id_map; // if we want to name our blobs we can store IDs here
	extern std::map<const blob_id_t, std::vector<std::byte>> blobs; // binary representation of stuff goes here.

	namespace BlobMgr{
		extern blob_id_t last_id_given;

		// Get the next ID value
		blob_id_t get_free_id();

		// Create empty blob of specific size
		blob_id_t create_blob_of_size(size_t size);
		std::vector<std::byte>& create_named_blob_of_size(const std::string&, size_t size);
		
		// Do we have a blob with this identifier?
		bool has(blob_id_t id);
		bool has(const std::string& name);

		// Getters
		std::vector<std::byte>& get(blob_id_t id);
		std::vector<std::byte>& get(std::string name);

		// Store container of data as a blob
		template<template <class, class...> class C, class T, class ...Ts>
		blob_id_t store(const C<T,Ts...>& container){
			size_t n_items = container.size();
			size_t n_bytes_per_item = sizeof(T);
			
			blob_id_t id = create_blob_of_size(n_items*n_bytes_per_item);
			std::vector<std::byte>& blob = blobs[id];
			size_t i=0;
			size_t j=0;
			for(const auto& item : container){
				for (j=0; j< n_bytes_per_item; ++j){
					blob[i*n_bytes_per_item + j] = *(static_cast<std::byte*>(&item)+j);
				}
				++i;
			}
		}
		// store container of data as a named blob
		template<template <class, class...> class C, class T, class ...Ts>
		void store_named(const std::string& key, const C<T,Ts...>& container){
			GET_LOGGER;
			LOG_DEBUG("Stored blob '%'", key);
			size_t n_items = container.size();
			size_t n_bytes_per_item = sizeof(T);
			
			std::vector<std::byte>& blob = create_named_blob_of_size(key, n_items*n_bytes_per_item);
			size_t i=0;
			size_t j=0;
			for(const auto& item : container){
				for (j=0; j< n_bytes_per_item; ++j){
					blob[i*n_bytes_per_item + j] = *(((std::byte*)(&item))+j);
				}
				++i;
			}
		}

		// Get a blob as a specific spanned type by id
		template<class T=uint8_t>
		std::span<T> get_as(blob_id_t blob_id){
			const std::vector<std::byte>& item = blobs[blob_id];
			return std::span<T>((T*)item.data(), (T*)(item.data()+item.size()));
		}
		// Get a blob as a specific spanned type by name
		template<class T=uint8_t>
		std::span<T> get_as(const std::string& blob_name){
			const std::vector<std::byte>& item = blobs[blob_name_to_id_map[blob_name]];
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
