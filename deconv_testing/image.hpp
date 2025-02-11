#ifndef __IMAGE_INCLUDED__
#define __IMAGE_INCLUDED__

#include <functional>
#include <vector>
#include "data_utils.hpp"

namespace du = data_utils;


enum class PixelFormatId{
	GREYSCALE,
	RGB,
	RGBA,
	UNDEFINED
};

enum class ChannelName{
	RED,
	GREEN,
	BLUE,
	ALPHA,
	BRIGHTNESS,
	UNDEFINED,
};

struct PixelFormat{
	PixelFormatId id;
	size_t channels_per_pixel;
	std::vector<ChannelName> channel_names;
};

// Internal image format is ALWAYS {R,R,R, ..., G,G,G, ..., B,B,B,...}
extern const PixelFormat UndefinedPixelFormat;
extern const PixelFormat GreyscalePixelFormat;
extern const PixelFormat RGBPixelFormat;
extern const PixelFormat RGBAPixelFormat;


struct Image{
	size_t write_head_idx;
	size_t read_head_idx;
	
	std::vector<size_t> shape; // x, y, z
	std::vector<double> data; // XXX... | RRR...GGG...BBB... | RRR...GGG...BBB...AAA...
	PixelFormat pxfmt;
	
	Image();
	Image(const std::vector<size_t>& _shape, const PixelFormat& _pxfmt);
	
	template<class T=void>
	void init_write(){
		write_head_idx=0;
	}
	
	template<class T>
	size_t copy_from(const T* data_ptr, size_t n, const std::function<double(const T*)>& norm_fn){
		// returns number of bytes copied
		size_t i = 0;
		for(i=0;i<n;++i,++write_head_idx){
			data.at(write_head_idx) = norm_fn(data_ptr+i);
		}
		return i*sizeof(T);
	}
	
	template<class T=void>
	void init_read(){
		read_head_idx = 0;
	}
	
	template<class T=void>
	std::span<double> get_span_of_layer(int z){
		size_t stride = shape[0]*shape[1];
		if((z+1)*stride > data.size()){
			std::cerr << "Error: Cannot read past end of file" << std::endl;
		}
		return std::span<double>(data.begin()+z*stride, data.begin()+(z+1)*stride);
	}
	
	template<class T=void>
	std::span<size_t> get_shape_of_layer(int z){
		return std::span<size_t>(shape.begin(), shape.begin()+2);
	}
};

#endif //__IMAGE_INCLUDED__


