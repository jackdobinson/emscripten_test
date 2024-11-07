#include "tiffio.h"
#include "file_like.hpp"
#include <vector>
#include "image.hpp"
#include <map>
#include "emscripten/val.h"

namespace Storage{

	using blob_id_t = size_t;

	std::map<std::string, Image> images;
	std::map<std::string, FileLike> filelikes;
	std::map<blob_id_t, std::vector<std::byte>> blobs; // binary representation of stuff goes here.

	namespace BlobMgr{
		blob_id_t last_id_given = 0;

		blob_id_t get_free_id(){
			blob_id_t t_id = last_id_given+1;
			while(blobs.contains(t_id)){
				++t_id;
			}
			last_id_given = t_id;
			return t_id;
		}

		template<class T=uint8_t>
		std::span<T> get_as(blob_id_t blob_id){
			const std::vector<std::byte>& item = blobs[blob_id];
			return std::span<T>((T*)item.data(), (T*)(item.data()+item.size()));
		}
	}



}


// See [this stack overflow answer for how to use TIFFClientOpen](https://stackoverflow.com/a/77007359)
enum class FileType {
	TIFF
};

struct FPtr{
	FileType ft;
	void* p;
};
// Should make this into a class
std::map<const std::string, FPtr> MemFileRegistry;

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

template<class T>
T* MemFileRegistry_get(const std::string& key){
	return (T*)(MemFileRegistry[key].p);
}


uint16_t TIFF_get_width(const std::string& name){
	GET_LOGGER;
	TIFF* tptr = MemFileRegistry_get<TIFF>(name);
	uint16_t image_width;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGEWIDTH, &image_width)){
		LOG_WARN("Could not determineimage_width for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	return image_width;
}

uint16_t TIFF_get_height(const std::string& name){
	GET_LOGGER;
	TIFF* tptr = MemFileRegistry_get<TIFF>(name);
	uint16_t ret;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGELENGTH, &ret)){
		LOG_WARN("Could not determine image length for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	return ret;
}

uint16_t TIFF_width(TIFF* tptr){
	GET_LOGGER;
	uint16_t image_width;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGEWIDTH, &image_width)){
		LOG_WARN("Could not determineimage_width for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	return image_width;
}

uint16_t TIFF_height(TIFF* tptr){
	GET_LOGGER;
	uint16_t ret;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGELENGTH, &ret)){
		LOG_WARN("Could not determine image length for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	return ret;
}
#define TTAG_STR(TAG) {(TAG), #TAG}


bool TIFF_dump_tags(TIFF* tptr){
	std::map <int, std::string> strip_tags = {
		TTAG_STR(TIFFTAG_IMAGEWIDTH),
		TTAG_STR(TIFFTAG_IMAGELENGTH),
		TTAG_STR(TIFFTAG_COMPRESSION),
		TTAG_STR(TIFFTAG_ROWSPERSTRIP),
		TTAG_STR(TIFFTAG_BITSPERSAMPLE),

	};
	GET_LOGGER;
	uint32_t tmp;

	for (const std::pair<int, std::string>& item : strip_tags){
		if (TIFFGetField(tptr, item.first, &tmp)){
			LOG_INFO("% %", item.second, tmp);
		} else {
			LOG_ERROR("Could not get tag %", item.second);
		}
	}
	return false; // should not happen
}

bool TIFF_uses_strips(TIFF* tptr){
	std::map <int, std::string> strip_tags = {
		//TTAG_STR(TIFFTAG_STRIPOFFSETS),
		//TTAG_STR(TIFFTAG_STRIPBYTECOUNTS),
		TTAG_STR(TIFFTAG_ROWSPERSTRIP) // only need one of these to be present to ensure we use strips
	};

	uint32_t tmp;

	for (const std::pair<int, std::string>& item : strip_tags){
		if (TIFFGetField(tptr, item.first, &tmp)){
			return true;
		} else {
			return false;
		}
	}
	return false; // should not happen
}
bool TIFF_uses_tiles(TIFF* tptr){
	std::map <int, std::string> strip_tags = {
		TTAG_STR(TIFFTAG_TILEWIDTH)
	};

	uint32_t tmp;

	for (const std::pair<int, std::string>& item : strip_tags){
		if (TIFFGetField(tptr, item.first, &tmp)){
			return true;
		} else {
			return false;
		}
	}
	return false; // should not happen
}


template<class T>
T stretch_range(T val, T old_min, T old_max, T new_min, T new_max){
	return ((val - old_min)/(old_max-old_min))*(new_max - new_min) + new_min;
}



template<class T>
std::vector<double> TIFF_read_strips_to_double(
		TIFF* tptr, 
		size_t n_strips, 
		size_t strip_size, 
		uint16_t planar_config, 
		uint16_t samples_per_pixel,
		double raw_min,
		double raw_max
	){
	GET_LOGGER;
	LOGV_DEBUG(typeid(T).name());
	LOGV_DEBUG(n_strips, strip_size, planar_config, samples_per_pixel, raw_min, raw_max);

	uint16_t image_width;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGEWIDTH, &image_width)){
		LOG_WARN("Could not determine image_width for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	uint16_t image_height;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGELENGTH, &image_height)){
		LOG_WARN("Could not determine image_height for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}



	size_t bytes_per_sample = sizeof(T);
	size_t n_items_per_strip = strip_size/bytes_per_sample;

	LOGV_DEBUG(bytes_per_sample, n_items_per_strip);

	std::vector<T> raw(image_width*image_height*samples_per_pixel, 0);

	std::byte* raw_data_ptr = (std::byte*)raw.data();
	size_t n_bytes_read;
	for(size_t i=0; i<n_strips; ++i){
		n_bytes_read = TIFFReadEncodedStrip(tptr, i, raw_data_ptr, -1);
		raw_data_ptr += n_bytes_read;
	}

	// Convert data to double
	uint16_t min_sample_value[samples_per_pixel];
	if (! TIFFGetField(tptr, TIFFTAG_SMINSAMPLEVALUE, &min_sample_value)){
		LOG_WARN("Could not determine min_sample_value for TIFF file");
		for (int i=0; i< samples_per_pixel; ++i){
			min_sample_value[i] = 0;
		}
	}
	
	uint16_t max_sample_value[samples_per_pixel];
	if (! TIFFGetField(tptr, TIFFTAG_SMAXSAMPLEVALUE, &max_sample_value)){
		LOG_WARN("Could not determine max_sample_value for TIFF file");
		for (int i=0; i< samples_per_pixel; ++i){
			max_sample_value[i] = 1;
		}
	}

	printf("min_sample_value = {"); for(size_t i=0;i<samples_per_pixel;i++){printf("%u, ", min_sample_value[i]);} printf("}\n");
	printf("max_sample_value = {"); for(size_t i=0;i<samples_per_pixel;i++){printf("%u, ", max_sample_value[i]);} printf("}\n");

	// Always return with samples within strips
	std::vector<double> data(raw.size());

	
	if (planar_config == 1){
		for(size_t i=0; i<data.size(); ++i){
			data[i] = stretch_range<double>(
				1.0*raw[i], 
				1.0*raw_min, 
				1.0*raw_max, 
				1.0*min_sample_value[i%samples_per_pixel], 
				1.0*max_sample_value[i%samples_per_pixel]
			);
		}
	}
	else if (planar_config == 2){
		LOG_WARN("Not tested PLANARCONFIG=2 yet");
		for (size_t i=0; i< data.size(); ++i){
			data[i] = stretch_range<double>(
				1.0*raw[i/3 +(i%samples_per_pixel)*image_width], 
				1.0*raw_min, 
				1.0*raw_max, 
				1.0*min_sample_value[i%samples_per_pixel], 
				1.0*max_sample_value[i%samples_per_pixel]
			);
		}
	}
	else {
		LOG_ERROR("Unknown planar config '' for TIFF file. Exiting...", planar_config);
		std::exit(EXIT_FAILURE);
	}

	LOG_DEBUG("Data packed as doubles...");

	return data;
}

std::vector<double> TIFF_read_strips(TIFF* tptr){
	// TIFFTAG_PLANARCONFIG = 1 then colour channels stored per pixel (i.e. each strip has {p1(RGB), p2(RGB),...}
	//                      = 2 then colour channels stored per strip (i.e. strip1={R R ...} strip2={G G ...} strip3={B B ...})
	GET_LOGGER;
	uint16_t planar_config;
	if (! TIFFGetField(tptr, TIFFTAG_PLANARCONFIG, &planar_config)){
		LOG_ERROR("Could not determine planar configuration for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	size_t n_strips = TIFFNumberOfStrips(tptr);
	size_t strip_size = TIFFStripSize(tptr);

	uint16_t bits_per_sample;
	if (! TIFFGetField(tptr, TIFFTAG_BITSPERSAMPLE, &bits_per_sample)){
		LOG_ERROR("Could not determine bits per sample for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	uint16_t samples_per_pixel;
	if (! TIFFGetField(tptr, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel)){
		LOG_WARN("Could not determine samples_per_pixel for TIFF file. Assuming 1");
		samples_per_pixel = 1;
	}

	uint16_t sample_format;
	if (! TIFFGetField(tptr, TIFFTAG_SAMPLEFORMAT, &sample_format)){
		LOG_ERROR("Could not determine sample_format for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	std::vector<double> data;

	switch (sample_format) {
		case 1:
			switch (bits_per_sample) {
				case 8:
					LOG_DEBUG("Reading Uint8");
					data = TIFF_read_strips_to_double<uint8_t>(tptr, n_strips, strip_size, planar_config, samples_per_pixel, 0.0, 255.0);
					break;
				case 16:
					LOG_DEBUG("Reading Uint16");
					data = TIFF_read_strips_to_double<uint16_t>(tptr, n_strips, strip_size, planar_config, samples_per_pixel, 0.0, 65535.0);
					break;
			}
			break;
		case 2:
			switch (bits_per_sample) {
				case 8:
					LOG_DEBUG("Reading Int8");
					data = TIFF_read_strips_to_double<int8_t>(tptr, n_strips, strip_size, planar_config, samples_per_pixel, -128., 127.);
					break;
				case 16:
					LOG_DEBUG("Reading Int16");
					data = TIFF_read_strips_to_double<int16_t>(tptr, n_strips, strip_size, planar_config, samples_per_pixel, -32768., 32767.);
					break;
			}
			break;
		case 3:
			switch (bits_per_sample){
				default:
					//data = TIFF_read_strips_to_double<double>(tptr, n_strips, strip_size, planar_config, samples_per_pixel);
					LOG_ERROR("Sample format 'double' for TIFF file not supported yet. Exiting...");
					std::exit(EXIT_FAILURE);
			}
			break;
		default:
			LOG_ERROR("Unknown sample format '%' for TIFF file. Exiting...", sample_format);
			std::exit(EXIT_FAILURE);
	}

	return data;
}


// NOTE: Going to have to read the TIFF using TIFFReadEncodedStrip and/or TIFFReadEncodedTile
// as we cannot assume 8 bits per pixel and RGBA pixel layouts.
// May have to write our own class that sits on top of libtiff and intelligently finds the
// number of bytes per sample, samples per pixel, and dumps uncompressed raw image data into memory.


std::string TIFF_from_js_array(const std::string& name, const emscripten::val& uint8_array){
	GET_LOGGER;

	std::vector<uint8_t> raw_data = emscripten::convertJSArrayToNumberVector<uint8_t>(uint8_array);
	LOGV_DEBUG(raw_data);

	std::byte* ptr =reinterpret_cast<std::byte*>(malloc(sizeof(uint8_t)*raw_data.size()));
	std::copy(reinterpret_cast<std::byte*>(raw_data.data()), reinterpret_cast<std::byte*>(raw_data.data()+raw_data.size()), ptr);

	FileLike* tiff_data = new FileLike(
		std::span<std::byte>(ptr, sizeof(uint8_t)*raw_data.size())
	);

	LOGV_DEBUG(tiff_data->bytes);

	if (MemFileRegistry.contains(name)){
		MemFileRegistry_delete(name);
	}
	
	TIFF* tptr = TIFFClientOpen(
		name.c_str(), 
		"rm", // r - read, m - don't memory map file
		(thandle_t)(tiff_data),
		FileLike::readproc,
		FileLike::writeproc,
		FileLike::seekproc,
		FileLike::closeproc,
		FileLike::sizeproc,
		FileLike::mapfileproc,
		FileLike::unmapfileproc
	);


	MemFileRegistry[name] = FPtr(
		FileType::TIFF,
		tptr
	);

	LOG_DEBUG("File % read in successfully", name);

	TIFF_dump_tags(tptr);

	if (!TIFF_uses_strips(tptr)){
		LOG_ERROR("Only TIFF files that use strips are supported at the moment. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	Storage::images.emplace(std::make_pair(
		name, 
		Image(
			std::vector<size_t>({TIFF_width(tptr), TIFF_height(tptr)}),
			TIFF_read_strips(tptr),
			GreyscalePixelFormat // Hard-coded for now	
		)
	));


	return name;
}


