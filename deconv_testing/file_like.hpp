#ifndef __FILE_LIKE_INCLUDED__
#define __FILE_LIKE_INCLUDED__

#include <cassert>
#include "logging.h"

/*
 * Defines an object that can be used like a file but operates in memory
*/

const uint64_t KiloByte = 2ULL<<10;
const uint64_t MegaByte = 2ULL<<20;
const uint64_t GigaByte = 2ULL<<30;
const uint64_t TeraByte = 2ULL<<40;
const uint64_t PetaByte = 2ULL<<50;
const uint64_t ExaByte  = 2ULL<<60;

struct FileLike{
	std::string file_name;
	std::vector<std::byte> bytes;
	size_t pos;
	bool is_open;
	bool contents_changed;
	
	FileLike();
	FileLike(const std::string& _file_name, const std::vector<std::byte>& _bytes);
	
	void set_open_with_contents(const std::vector<std::byte>& new_bytes, bool _contents_changed=false);
	
	
	template<class PTR>
	static FileLike& cast_from_ptr(PTR p){
		return *(static_cast<FileLike*>(static_cast<void*>(p)));
	}

	template<class PTR, class BUF, class SIZE>
	static SIZE readproc(PTR fptr, BUF buf, SIZE size){
		FileLike& fp = cast_from_ptr(fptr);
		assert(fp.is_open);
		
		std::byte* bbuf = (std::byte*)(buf);
		size_t i=0;
		size_t j=fp.pos;
		for (i=0; i<size && j<fp.bytes.size(); ++i,++j){
			bbuf[i] = fp.bytes[j];
		}
		const size_t n_read = i;
		fp.pos += n_read;
		return n_read;
	}

	template<class PTR, class BUF, class SIZE>
	static SIZE writeproc(PTR fptr, BUF buf, SIZE size){
		GET_LOGGER;
		
		FileLike& fp = cast_from_ptr(fptr); // cast pointer as a filelike
		assert(fp.is_open);
		std::byte* bbuf = (std::byte*)(buf); // cast the buffer as a string of bytes
		
		size_t old_pos = fp.pos;
		size_t i=0; // index of current buffer byte
		size_t j=fp.pos; // index of current file byte
		
		if (fp.pos + size > fp.bytes.size()){
			LOG_ERROR(
				"Cannot write past end of span for filelike object. Holder size is %, want to write % bytes from location % to location %.", 
				fp.bytes.size(), size, fp.pos, fp.pos+size
			);
			return -1;
		}
		
		for(i=0; i<size; ++i,++j){
			fp.bytes[j] = bbuf[i];
		}
		fp.pos += i;
		
		if (fp.pos != old_pos){ // if any data was written, the contents is now changed
			fp.contents_changed = true;
		}
		return i;
	}

	template<class PTR, class OFFSET>
	static OFFSET seekproc(PTR fptr, OFFSET offset, int whence){
		GET_LOGGER;
		//LOG_DEBUG("Seeking file % offset % whence %", fptr, offset, whence);

		size_t new_pos;
		FileLike& fp = cast_from_ptr(fptr);
		assert(fp.is_open);

		switch (whence) {
			case (SEEK_SET):
				new_pos = offset;
				break;
			case (SEEK_CUR):
				new_pos = fp.pos + offset;
				break;
			case (SEEK_END):
				new_pos = fp.bytes.size() + offset;
				break;
			default:
				LOG_ERROR("Unknown 'whence' parameter when seeking TIFF file '%'. Accepted values are SEEK_SET=% SEEK_CUR=% SEEK_END=%", whence, SEEK_SET, SEEK_CUR, SEEK_END);
				return -1;
		}

		if (new_pos > fp.bytes.size()){
			LOG_ERROR("Cannot seek past the end of a file");
			return -1;
		}

		fp.pos = new_pos;
		return new_pos;
	}

	template<class PTR>
	static int closeproc(PTR fptr){
		// NOTE: I think this might be causing some problems
		
		GET_LOGGER;
		//LOG_DEBUG("Closing file %", fptr);
		FileLike& fp = cast_from_ptr(fptr);
		assert(fp.is_open);
		fp.is_open=false;
		
		// If we have altered the data
		if(fp.contents_changed){
			LOG_DEBUG("Contents of FileLike for '%' have changed, we should write altered data.", fp.file_name);
		} else {
			LOG_DEBUG("Contents of FileLike for '%' have NOT changed.", fp.file_name);
		}
		
		
		return 0;
	}

	template<class PTR, class OFFSET>
	static OFFSET sizeproc(PTR fptr){
		FileLike& fp = cast_from_ptr(fptr);
		assert(fp.is_open);
		return fp.bytes.size();
	}
	
	template<class PTR, class BUF, class OFFSET>
	static int mapfileproc(PTR fptr, BUF* buf, OFFSET* size){
		GET_LOGGER;
		FileLike& fp = cast_from_ptr(fptr);
		assert(fp.is_open);
		LOG_WARN("memory mapping not supported");
		return -1;
	}

	template<class PTR, class BUF, class OFFSET>
	static void unmapfileproc(PTR fptr, BUF buf, OFFSET size){
		GET_LOGGER;
		FileLike& fp = cast_from_ptr(fptr);
		assert(fp.is_open);
		LOG_WARN("un-memory mapping not supported");
		return;
	}


};


#endif //__FILE_LIKE_INCLUDED__
