#ifndef __JS_GLUE_INCLUDED__
#define __JS_GLUE_INCLUDED__

#include <cmath>

#include "storage.hpp"
#include "image.hpp"


template<class T>
T round_to(double v){
	return (T)(v + (v > 0 ? 0.5 : -0.5));
}

template<class T>
T stretch_range(
		T val, 
		T old_min, 
		T old_max, 
		T new_min, 
		T new_max
	){
	return ((val - old_min)/(old_max-old_min))*(new_max - new_min) + new_min;
}




template<class R=std::byte, class T>
std::span<R> image_as_blob(
		const std::string& name, 
		const std::vector<T>& a, 
		const PixelFormat input_pixel_format=GreyscalePixelFormat,
		const std::string& name_tag = "__image_data"
	){
	GET_LOGGER;

	std::string image_name = name + name_tag;

	LOGV_DEBUG(image_name, a, input_pixel_format.channels_per_pixel);
	LOGV_DEBUG(a.size());
	
	
	T min = a[0], max=a[0];
	for(const T& item : a){
		if (item > max) max = item;
		if (item < min) min = item;
	}

	const PixelFormat output_pixel_format = RGBAPixelFormat;
	size_t size_of_image_data = round_to<int>((1.0*output_pixel_format.channels_per_pixel*a.size())/input_pixel_format.channels_per_pixel);

	Storage::named_blobs[image_name] = std::vector<std::byte>(size_of_image_data);
	std::vector<std::byte>& image_data = Storage::named_blobs[image_name];

	LOGV_DEBUG(image_data.size(), size_of_image_data);
	assert(image_data.size() == size_of_image_data);

	switch (input_pixel_format.id) {
		case PixelFormatId::GREYSCALE :{ // input pixels {XXX...}
			LOG_DEBUG("PixelFormatId::GREYSCALE");
			size_t j=0;
			for (size_t i=0; i< a.size(); ++i){
				j = 4*i;
				image_data[j] = (std::byte)(round_to<uint8_t>(stretch_range(a[i], min, max, 0.0, 255.0)));

				image_data[j+1] = image_data[j];
				image_data[j+2] = image_data[j];
				image_data[j+3] = (std::byte)(255);
			}
			break;
		}
		case PixelFormatId::RGB :{ // input pixels {RRR...GGG...BBB...}
			LOG_DEBUG("PixelFormatId::RGB");
			LOGV_DEBUG(a.size(), min, max);
			size_t layer_stride = a.size()/3;
			LOGV_DEBUG(layer_stride);
			size_t k = 0;
			for (k=0;k<3;++k){
				for (size_t i=0; i < layer_stride; ++i){
					//if (i< 20){
					//	LOGV_DEBUG(i, k);
					//	LOGV_DEBUG(4*i+k);
					//	LOGV_DEBUG(layer_stride*k + i);
					//	LOGV_DEBUG(a[layer_stride*k + i]);
					//}
					image_data[4*i+k] = (std::byte)(round_to<uint8_t>(stretch_range(a[layer_stride*k + i], min, max, 0.0, 255.0)));
				}
			}
			for (size_t i=0; i < layer_stride; ++i){
				image_data[4*i+3] = (std::byte)(255);
			}
			LOGV_DEBUG(image_data);
			break;
		}
		case PixelFormatId::RGBA :{ // input pixels {RRR...GGG...BBB...AAA...}
			LOG_DEBUG("PixelFormatId::RGBA");
			size_t j=0;
			size_t k=0;
			for (size_t i=0; i < a.size(); ++i){
				j = (i%4)*a.size()/4;
				k = i%(a.size()/4);
				image_data[i] = (std::byte)(round_to<uint8_t>(stretch_range(a[j+k], min, max, 0.0, 255.0)));
			}
			break;
		}

		default:{
			std::cerr << "Unknown pixel format id" << std::endl;
			std::exit(EXIT_FAILURE);
			break;
		}
	}

	return std::span<R>((R*)(image_data.data()), (R*)(image_data.data()+image_data.size()));
}


template<class T>
emscripten::val vector_as_JSImageData(
		const std::string& name, 
		const std::vector<T>& a, 
		const PixelFormat input_pixel_format=GreyscalePixelFormat
	){
	//GET_LOGGER;
	//LOGV_DEBUG(a.data());

	std::span<uint8_t> image_data = image_as_blob<uint8_t,T>(name, a, input_pixel_format);
	return emscripten::val(emscripten::typed_memory_view(image_data.size(), image_data.data()));
}


template<class T>
void dump_histogram(
		std::vector<T> a,
		size_t nbins=100
	){
	std::sort(a.begin(), a.end());

	std::vector<double> top_edges(nbins);
	std::vector<uint32_t> counts(nbins, 0);

	T a_min=a.front(), a_max=a.back();

	for (size_t i=0;i<top_edges.size(); ++i){
		top_edges[i] = a_min + ((1.0*i)/top_edges.size())*(a_max - a_min);
	}

	size_t j=0;
	for(const T& item : a){
		while (item > top_edges[j]){
			++j;
		}
		++(counts[j]);
	}

	size_t n_counts=0;
	for(auto& item : counts){
		n_counts += item;
	}

	std::string output="HISTOGRAM: N-counts " + std::to_string(n_counts) + "\n";
	for(size_t i=0; i<top_edges.size(); ++i){
		output += "    " + std::to_string(counts[i]) + "\n" + std::to_string(top_edges[i]) + "\n";
	}

	printf("%s", output.c_str());
}


inline std::string Make_Gaussian_Image(const std::string& name, const float px_angular_size, const float sigma){
	GET_LOGGER;
	
	std::string msg="";
	LOG_DEBUG("Generating gaussian image '%'", name);
	LOGV_DEBUG(px_angular_size, sigma);
	const float sigma_px = sigma/px_angular_size;
	LOGV_DEBUG(sigma_px);
	uint16_t nx = 10 * sigma_px;
	nx += 1 - (nx % 2);
	const uint16_t ny = nx;
	const uint16_t cx = nx/2;
	const uint16_t cy = ny/2;

	float dx, dy;
	float factor = 1/(sigma_px*sqrt(2*M_PI));

	std::vector<double> image_data(nx*ny);
	
	for(int i=0;i<nx;++i){
		dx = i - cx;
		for(int j=0;j<ny;++j){
			
			dy = j - cy;
			image_data[i*ny + j] = factor * exp(-0.5*(dx*dx + dy*dy)/(sigma_px*sigma_px));
		}
	}
	
	Storage::images.emplace(
		std::make_pair(name, Image())
	);
	
	Image& image = Storage::images[name];
	image.pxfmt = GreyscalePixelFormat;
	image.data = image_data;
	image.shape = std::vector<size_t>{nx, ny, 1};
	
	return msg;
}

inline uint32_t Image_get_height(const std::string& name){
	GET_LOGGER;
	LOG_DEBUG("Getting height of Image '%'", name);
	Image& image = Storage::images[name];
	
	return image.shape[1];
}

inline uint32_t Image_get_width(const std::string& name){
	GET_LOGGER;
	LOG_DEBUG("Getting height of Image '%'", name);
	Image& image = Storage::images[name];
	
	return image.shape[0];
}




#endif //__JS_GLUE_INCLUDED__
