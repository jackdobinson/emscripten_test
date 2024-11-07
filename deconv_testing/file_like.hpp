#ifndef __FILE_LIKE_INCLUDED__
#define __FILE_LIKE_INCLUDED__

#include "logging.h"

/*
 * Defines an object that can be used like a file but operates in memory
*/
struct FileLike{
	std::span<std::byte> bytes;
	size_t pos = 0;

	template<class PTR, class BUF, class SIZE>
	static SIZE readproc(PTR fptr, BUF buf, SIZE size){
		FileLike& mc = *((FileLike*)(fptr));
		std::byte* bbuf = (std::byte*)(buf);
		size_t i=0;
		size_t j=mc.pos;
		for (i=0; i<size && j<mc.bytes.size(); ++i,++j){
			bbuf[i] = mc.bytes[j];
		}
		const size_t n_read = i;
		mc.pos += n_read;
		return n_read;
	}

	template<class PTR, class BUF, class SIZE>
	static SIZE writeproc(PTR fptr, BUF buf, SIZE size){
		GET_LOGGER;
		LOG_WARN("Writing to file not supported");
		return -1;
	}

	template<class PTR, class OFFSET>
	static OFFSET seekproc(PTR fptr, OFFSET offset, int whence){
		GET_LOGGER;
		//LOG_DEBUG("Seeking file % offset % whence %", fptr, offset, whence);

		size_t new_pos;
		FileLike& mc = *((FileLike*)(fptr));

		switch (whence) {
			case (SEEK_SET):
				new_pos = offset;
				break;
			case (SEEK_CUR):
				new_pos = mc.pos + offset;
				break;
			case (SEEK_END):
				new_pos = mc.bytes.size() + offset;
				break;
			default:
				LOG_ERROR("Unknown 'whence' parameter when seeking TIFF file '%'. Accepted values are SEEK_SET=% SEEK_CUR=% SEEK_END=%", whence, SEEK_SET, SEEK_CUR, SEEK_END);
				return -1;
		}

		if (new_pos > mc.bytes.size()){
			LOG_ERROR("Cannot seek past the end of a file");
			return -1;
		}

		mc.pos = new_pos;
		return new_pos;
	}

	template<class PTR>
	static int closeproc(PTR fptr){
		//GET_LOGGER;
		//LOG_DEBUG("Closing file %", fptr);
		FileLike* mc = (FileLike*)fptr;
		// NOTE: because of the peculiar way that emscripten passes chunks of memory to the program, should free
		// the bytes here, then delete the memory chunk.
		free (mc->bytes.data());
		delete mc;
		return 0;
	}

	template<class PTR, class OFFSET>
	static OFFSET sizeproc(PTR fptr){
		return ((FileLike*)(fptr))->bytes.size();
	}
	
	template<class PTR, class BUF, class OFFSET>
	static int mapfileproc(PTR fptr, BUF* buf, OFFSET* size){
		GET_LOGGER;
		LOG_WARN("memory mapping not supported");
		return -1;
	}

	template<class PTR, class BUF, class OFFSET>
	static void unmapfileproc(PTR fptr, BUF buf, OFFSET size){
		GET_LOGGER;
		LOG_WARN("un-memory mapping not supported");
		return;
	}


};


#endif //__FILE_LIKE_INCLUDED__
