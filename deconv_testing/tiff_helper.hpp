#ifndef __TIFF_HELPER_INCLUDED__
#define __TIFF_HELPER_INCLUDED__

#include "tiffio.h"
#include "file_like.hpp"
#include <vector>
#include "logging.h"
#include "emscripten/val.h"
#include "storage.hpp"
#include "js_glue.hpp"
#include "image.hpp"


// See [this stack overflow answer for how to use TIFFClientOpen](https://stackoverflow.com/a/77007359)
#define TTAG_STR(TAG) {(TAG), #TAG}

uint16_t TIFF_get_width(const std::string& name);
uint16_t TIFF_get_height(const std::string& name);
uint16_t TIFF_width(TIFF* tptr);
uint16_t TIFF_height(TIFF* tptr);

bool TIFF_dump_tags(TIFF* tptr);
bool TIFF_uses_strips(TIFF* tptr);
bool TIFF_uses_tiles(TIFF* tptr);


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

std::vector<double> TIFF_read_strips(TIFF* tptr);

std::string TIFF_from_js_array(const std::string& name, const emscripten::val& uint8_array);



#endif //__TIFF_HELPER_INCLUDED__
