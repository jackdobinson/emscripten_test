#include "storage.hpp"

std::map<const std::string, FPtr> MemFileRegistry;

namespace Storage {

std::map<std::string, Image> images;
std::map<std::string, FileLike> filelikes;
std::map<blob_id_t, std::vector<std::byte>> blobs; // binary representation of stuff goes here.

blob_id_t BlobMgr::last_id_given=0;
}


Storage::blob_id_t Storage::BlobMgr::get_free_id(){
	blob_id_t t_id = last_id_given+1;
	while(blobs.contains(t_id)){
		++t_id;
	}
	last_id_given = t_id;
	return t_id;
}

int MemFileRegistry_delete(const std::string& key){
	GET_LOGGER;
	if(!MemFileRegistry.contains(key)){
		return -1;
	}
	const FPtr& fptr = MemFileRegistry[key];
	switch (fptr.ft){
		case (FileType::TIFF):
			TIFFClose((TIFF*)fptr.p);
			break;
		default:
			LOG_ERROR("Unknown FileType to cast pointer to");
			return -1;
	}
	return 0;
}
void MemFileRegistry_destroy(){
	for (const std::pair<std::string, FPtr>& item : MemFileRegistry){
		MemFileRegistry_delete(item.first);
	}
}


