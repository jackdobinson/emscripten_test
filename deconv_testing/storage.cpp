#include "storage.hpp"

std::map<const std::string, FPtr> MemFileRegistry;

namespace Storage {

	std::map<std::string, Image> images;
	std::map<std::string, FileLike> filelikes;

	std::map<const std::string, blob_id_t> blob_name_to_id_map; // if we want to name our blobs, store ID in here
	std::map<const blob_id_t, std::vector<std::byte>> blobs; // binary representation of stuff goes here.

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

Storage::blob_id_t Storage::BlobMgr::create_blob_of_size(size_t size){
	blob_id_t id = get_free_id();
	blobs.emplace(
		id,
		size
	);
	return id;
}

std::vector<std::byte>& Storage::BlobMgr::create_named_blob_of_size(const std::string& name, size_t size){
	// replace blob that already exists with this name
	// re-use the ID number
	GET_LOGGER;
	LOG_DEBUG("Creating named blob '%'", name);
	blob_id_t id;
	if(blob_name_to_id_map.contains(name)){
		LOG_WARN("Named blob '%' already exists, creating a NEW blob of size % in its place", name, size);
		id = blob_name_to_id_map[name];
		LOG_DEBUG("Reusing previous id '%'", id);
		for(const auto [key, value] : blobs){
			LOGV_DEBUG(key, value);
		}
		std::vector<std::byte> temp(size);
		blobs.insert_or_assign(id, temp);
		LOG_DEBUG("New vector assigned to id");
	} else {
		id = create_blob_of_size(size);
		blob_name_to_id_map[name] = id;
	}
	LOG_DEBUG("Blob created.");
	return blobs[id];
}

// Do we have a blob with this identifier?
bool Storage::BlobMgr::has(blob_id_t id){
	return blobs.contains(id);
}
bool Storage::BlobMgr::has(const std::string& name){
	GET_LOGGER;
	bool has_name = blob_name_to_id_map.contains(name);
	
	if(has_name){
		bool has_id = blobs.contains(blob_name_to_id_map[name]);
		if(!has_id){
			LOG_ERROR("We have a blob name '%' that does not correspond to and ID, this should never happen", name);
			exit(EXIT_FAILURE);
		}
	}
	return has_name;
}


// Getters
std::vector<std::byte>& Storage::BlobMgr::get(blob_id_t id){
	return blobs[id];
}
std::vector<std::byte>& Storage::BlobMgr::get(std::string name){
	return blobs[blob_name_to_id_map[name]];
}


int MemFileRegistry_delete(const std::string& key){
	GET_LOGGER;
	if(!MemFileRegistry.contains(key)){
		LOG_DEBUG("Key '%' not in MemFileRegistry", key);
		return -1;
	}
	const FPtr& fptr = MemFileRegistry[key];
	switch (fptr.ft){
		case (FileType::TIFF):
			LOG_DEBUG("Closing TIFF file '%'", key);
			TIFFClose((TIFF*)(fptr.p));
			break;
		default:
			LOG_ERROR("Unknown FileType to cast pointer to");
			return -1;
	}
	MemFileRegistry.erase(key);
	return 0;
}
void MemFileRegistry_destroy(){
	for (const std::pair<std::string, FPtr>& item : MemFileRegistry){
		MemFileRegistry_delete(item.first);
	}
}


