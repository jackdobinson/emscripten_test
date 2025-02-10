#ifndef __TIFF_HELPER_INCLUDED__
#define __TIFF_HELPER_INCLUDED__

#include <cstdio>
#include <vector>
#include <functional>
#include "tiffio.h"
#include "file_like.hpp"
#include "logging.h"
#include "emscripten/val.h"
#include "storage.hpp"
#include "js_glue.hpp"
#include "image.hpp"
#include "data_utils.hpp"

#include "emscripten.h"


// See [this stack overflow answer for how to use TIFFClientOpen](https://stackoverflow.com/a/77007359)
#define TTAG_STR(TAG) {(TAG), #TAG}

namespace du = data_utils;

uint32_t TIFF_get_width(const std::string& name);
uint32_t TIFF_get_height(const std::string& name);
uint32_t TIFF_width(TIFF* tptr);
uint32_t TIFF_height(TIFF* tptr);

bool TIFF_dump_tags(TIFF* tptr);
bool TIFF_uses_strips(TIFF* tptr);
bool TIFF_uses_tiles(TIFF* tptr);

TIFF* TIFF_open(const std::string& name, const char* mode);
void TIFF_close(const std::string& name);

struct TIFF_Ptr{
	// Attempts to get a TIFF* pointer, first by looking for a valid stored pointer,
	// second by opening a stored file. Will release the pointer when going out of scope.
	std::string name;
	TIFF* p;
	bool should_close;
	
	TIFF_Ptr(const std::string& _name, const char* mode="r+m");
	~TIFF_Ptr();
	operator TIFF*()const;
};

struct TIFF_TileInfo{
	size_t n_tiles;
	size_t tile_size;
	size_t rows_per_tile;
	
	TIFF_TileInfo(TIFF* tptr);
};

struct TIFF_StripInfo{
	size_t n_strips;
	size_t strip_size;
	size_t rows_per_strip;
	
	TIFF_StripInfo(TIFF* tptr);
};

struct TIFF_LayoutInfo{
	size_t n_items;
	size_t item_size;
	size_t rows_per_item;
	
	void fill_from(const TIFF_StripInfo& other);
	void fill_from(const TIFF_TileInfo& other);
};

struct TIFF_PixelInfo{
	uint16_t photometric_interpretation;
	uint16_t planar_config;
	uint16_t samples_per_pixel;
	
	bool uses_strips;
	bool uses_tiles;
	
	std::vector<uint16_t> bits_per_sample;
	std::vector<uint16_t> sample_format;
	std::vector<double> min_sample_value;
	std::vector<double> max_sample_value;
	
	TIFF_PixelInfo(TIFF* tptr);
};

struct TIFF_ImageShape{
	uint16_t width;
	uint16_t height;
	
	TIFF_ImageShape(TIFF* tptr);
};

std::vector<double> TIFF_read_strips(TIFF* tptr);

std::string TIFF_from_js_array(const std::string& name, const emscripten::val& uint8_array);


template<class T>
std::vector<T> TIFF_doubles_to_samples_as(
		const TIFF_PixelInfo& pixel_info, 
		const std::vector<double>& data,
		const PixelFormat& pixel_format, 
		double min_pack_value,
		double max_pack_value
	){
	GET_LOGGER;
	LOG_DEBUG("Converting greyscale doubles to format we can write to TIFF file...");
	LOGV_DEBUG(
		pixel_info.planar_config,
		pixel_info.samples_per_pixel,
		pixel_info.uses_strips,
		pixel_info.uses_tiles,
		pixel_info.bits_per_sample,
		pixel_info.sample_format,
		pixel_info.min_sample_value,
		pixel_info.max_sample_value
	);
	LOGV_DEBUG(typeid(T).name());
	
	size_t n_samples = pixel_info.samples_per_pixel*data.size();
	std::vector<T> sample_data(n_samples);
	LOGV_DEBUG(pixel_info.samples_per_pixel);
	
	switch(pixel_format.id){
	
		
		case PixelFormatId::GREYSCALE:{
		
			switch(pixel_info.planar_config){
				case 1:{ // RGBA RGBA RGBA ...
					LOG_DEBUG("Planar config = 1");
					for(size_t i=0; i<n_samples; ++i){
						sample_data[i] = stretch_range<double>(
							data[i/pixel_info.samples_per_pixel],
							pixel_info.min_sample_value[i%pixel_info.samples_per_pixel],
							pixel_info.max_sample_value[i%pixel_info.samples_per_pixel],
							min_pack_value,
							max_pack_value
						);
					}
					break;
				}
				case 2: {// R R R R... G G G G... B B B B... A A A A...
					LOG_DEBUG("Planar config = 2");
					for(size_t i=0; i<n_samples; ++i){
						sample_data[i/pixel_info.samples_per_pixel + (i%pixel_info.samples_per_pixel)*data.size()] = stretch_range<double>(
							data[i/pixel_info.samples_per_pixel],
							pixel_info.min_sample_value[i%pixel_info.samples_per_pixel],
							pixel_info.max_sample_value[i%pixel_info.samples_per_pixel],
							min_pack_value,
							max_pack_value
						);
					}
					break;
				}
				default: {
					puts("Error: Unknown planar config value, Exiting...");
					//LOG_ERROR("Unknown planar config value '%'", pixel_info.planar_config);
					exit(EXIT_FAILURE);
				}
			}
		}
		case PixelFormatId::RGB:{
			puts("Error: RGBPixelFormat pixel format not supported yet. Exiting...");
			exit(EXIT_FAILURE);
		}
		case PixelFormatId::RGBA:{
			puts("Error: RGBAPixelFormat pixel format not supported yet. Exiting...");
			exit(EXIT_FAILURE);
		}
		default:{
			puts("Error: Unknown pixel format. Exiting...");
			exit(EXIT_FAILURE);
		}
		
	}
	return sample_data;
}

template<class T>
void TIFF_write_strips_from(
		TIFF* tptr,
		const TIFF_LayoutInfo& layout_info,
		const std::vector<T>& sample_data
	){
	
	std::byte* data = (std::byte*)(sample_data.data());
	std::byte* data_end = (std::byte*)(sample_data.data()+sample_data.size());
	
	// Should have the same number of bytes in our sample data as we expect to write
	assert((data_end - data) == (layout_info.n_items * layout_info.item_size));
	
	size_t i=0;
	while(data < data_end){
		TIFFWriteEncodedStrip(tptr, i, data, layout_info.item_size);
		data += layout_info.item_size;
		++i;
	}
}

template<class T>
void TIFF_write_strips(
		TIFF* tptr, 
		TIFF_LayoutInfo layout_info,
		TIFF_PixelInfo pixel_info,
		const std::vector<T>& data, // always greyscale, 1 value per pixel
		const PixelFormat& pixel_format
	){
	GET_LOGGER;
	LOG_DEBUG("Writing strips to file...");
	LOGV_DEBUG(
		layout_info.n_items,
		layout_info.item_size,
		layout_info.rows_per_item
	);
	// NOTE: Only greyscale input data is supported
	
	switch (pixel_info.sample_format[0]) {
		case 1:
			switch (pixel_info.bits_per_sample[0]) {
				case 8:
					LOG_DEBUG("Writing Uint8");
					TIFF_write_strips_from(tptr, layout_info, TIFF_doubles_to_samples_as<uint8_t>(pixel_info, data, pixel_format, 0.0, 255.0));
					break;
				case 16:
					LOG_DEBUG("Writing Uint16");
					TIFF_write_strips_from(tptr, layout_info, TIFF_doubles_to_samples_as<uint16_t>(pixel_info, data, pixel_format,  0.0, 65535.0));
					break;
			}
			break;
		case 2:
			switch (pixel_info.bits_per_sample[0]) {
				case 8:
					LOG_DEBUG("Writing Int8");
					TIFF_write_strips_from(tptr, layout_info, TIFF_doubles_to_samples_as<int8_t>(pixel_info, data, pixel_format,  -128., 127.));
					break;
				case 16:
					LOG_DEBUG("Writing Int16");
					TIFF_write_strips_from(tptr, layout_info, TIFF_doubles_to_samples_as<int16_t>(pixel_info, data, pixel_format,  -32768., 32767.));
					break;
			}
			break;
		case 3:
			switch (pixel_info.bits_per_sample[0]){
				default:
					//data = TIFF_read_strips_to_double<double>(tptr, n_strips, strip_size, planar_config, samples_per_pixel);
					puts("Error: Sample format 'double' for TIFF file not supported yet. Exiting...");
					std::exit(EXIT_FAILURE);
			}
			break;
		default:
			std::cerr << "Error: Unsupported sample format " << pixel_info.sample_format << " for TIFF file. Exiting..." << std::endl;
			std::exit(EXIT_FAILURE);
	}
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
	// NOTE: only greyscale output data is supported
	GET_LOGGER;
	LOGV_DEBUG(typeid(T).name());
	LOGV_DEBUG(n_strips, strip_size, planar_config, samples_per_pixel, raw_min, raw_max);

	uint32_t image_width;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGEWIDTH, &image_width)){
		puts("Error: Could not determine image_width for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	uint32_t image_height;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGELENGTH, &image_height)){
		puts("Error: Could not determine image_height for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}


	size_t bytes_per_sample = sizeof(T);
	size_t n_items_per_strip = strip_size/bytes_per_sample;
	LOGV_DEBUG(bytes_per_sample, n_items_per_strip, image_width, image_height);

	std::vector<T> raw(image_width*image_height*samples_per_pixel, 0);

	std::byte* raw_data_ptr = (std::byte*)(raw.data());
	size_t n_bytes_read;
	for(size_t i=0; i<n_strips; ++i){
		LOGV_DEBUG(i, n_strips, raw_data_ptr);
		n_bytes_read = TIFFReadEncodedStrip(tptr, i, raw_data_ptr, -1);
		LOGV_DEBUG(n_bytes_read);
		raw_data_ptr += n_bytes_read;
	}
	
	// Convert data to double
	uint16_t min_sample_value[samples_per_pixel];
	if (! TIFFGetField(tptr, TIFFTAG_SMINSAMPLEVALUE, &min_sample_value)){
		puts("Warning: Could not determine min_sample_value for TIFF file, setting to 0");
		for (int i=0; i< samples_per_pixel; ++i){
			min_sample_value[i] = 0;
		}
	}
	uint16_t max_sample_value[samples_per_pixel];
	if (! TIFFGetField(tptr, TIFFTAG_SMAXSAMPLEVALUE, &max_sample_value)){
		puts("Warning: Could not determine max_sample_value for TIFF file, setting to 1");
		for (int i=0; i< samples_per_pixel; ++i){
			max_sample_value[i] = 1;
		}
	}

	//printf("min_sample_value = {"); for(size_t i=0;i<samples_per_pixel;i++){printf("%u, ", min_sample_value[i]);} printf("}\n");
	//printf("max_sample_value = {"); for(size_t i=0;i<samples_per_pixel;i++){printf("%u, ", max_sample_value[i]);} printf("}\n");

	// Always return with samples within strips
	std::vector<double> data(raw.size());

	
	if (planar_config == 1){ // RGB,RGB,RGB,...
		size_t stride = data.size()/samples_per_pixel;
		size_t j=0;
		size_t k=0;
		for(size_t i=0; i<data.size(); ++i){
			j = i%samples_per_pixel;
			k = i%stride; 
			data[i*stride + k] = stretch_range<double>(
				1.0*raw[i], 
				1.0*raw_min, 
				1.0*raw_max, 
				1.0*min_sample_value[i%samples_per_pixel], 
				1.0*max_sample_value[i%samples_per_pixel]
			);
		}
	}
	else if (planar_config == 2){ //RRR...GGG...BBB...
		puts("Warning: Not tested PLANARCONFIG=2 yet");
		for (size_t i=0; i< data.size(); ++i){
			data[i] = stretch_range<double>(
				1.0*raw[i], 
				1.0*raw_min, 
				1.0*raw_max, 
				1.0*min_sample_value[i%samples_per_pixel], 
				1.0*max_sample_value[i%samples_per_pixel]
			);
		}
	}
	else {
		std::cerr << "Error: Unknown planar config " << planar_config << " for TIFF file. Exiting..." << std::endl;
		std::exit(EXIT_FAILURE);
	}

	LOG_DEBUG("Data packed as doubles...");
	return data;
}

template<class T=double>
std::span<uint8_t> TIFF_bytes_like(
		const std::string& original_file_name, 
		const std::string& file_name,
		const std::vector<T>& data,
		const PixelFormat data_pixel_format
	){
	GET_LOGGER;
	LOGV_DEBUG(data.size());
	// Create a TIFF with as much of the same structure as `original_file_name` as possible
	
	// Copy original file data to new file
	Storage::named_blobs[file_name] = Storage::named_blobs.at(original_file_name);
	
	
	// Open as read and write
	TIFF* tptr = TIFF_open(file_name, "r+m");
	
	if (!TIFF_uses_strips(tptr)){
		puts("Error: Only TIFF files that use strips are supported at the moment. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	
	// Get pixel format etc.
	LOG_DEBUG("Getting image shape, pixel info, layout info, etc. of original image");
	TIFF_ImageShape image_shape(tptr);
	TIFF_PixelInfo pixel_info(tptr);
	TIFF_LayoutInfo layout_info;
	
	if(pixel_info.uses_strips){
		layout_info.fill_from(TIFF_StripInfo(tptr));
	}
	else if (pixel_info.uses_tiles){
		layout_info.fill_from(TIFF_TileInfo(tptr));
	}
	else {
		puts("Error: Cannot determine if TIFF uses tile or strip layout. This should never happen");
		exit(EXIT_FAILURE);
	}
	
	// update min and max sample values
	for (size_t i=0; i<pixel_info.samples_per_pixel; ++i){
		LOGV_DEBUG(du::min(data));
		LOGV_DEBUG(du::max(data));
		LOGV_DEBUG(du::sum(data));
		pixel_info.min_sample_value[i] = du::min(data);
		pixel_info.max_sample_value[i] = du::max(data);
	}
	
	
	// Write new data into copied file.
	if(pixel_info.uses_strips){
		TIFF_write_strips(
			tptr,
			layout_info,
			pixel_info,
			data,
			data_pixel_format
		);
	}
	else if (pixel_info.uses_tiles){
		puts("Error:TIFF tile layout unsupported");
		exit(EXIT_FAILURE);
	}
	else {
		puts("Error: TIFF layout cannot be determined, must be strips or tiles");
		exit(EXIT_FAILURE);
	}
	
	
	TIFF_close(file_name);
	
	// Get the written file as a span of uint8_t
	std::vector<std::byte>& raw_data = Storage::named_blobs[file_name];

	return std::span<uint8_t>((uint8_t*)(raw_data.data()), (uint8_t*)(raw_data.data()+raw_data.size()));
}


#endif //__TIFF_HELPER_INCLUDED__
