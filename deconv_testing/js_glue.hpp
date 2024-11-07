#ifndef __JS_GLUE_INCLUDED__
#define __JS_GLUE_INCLUDED__

#include "storage.hpp"
#include "image.hpp"


template<class T>
T round_to(double v){
	return (T)(v + (v > 0 ? 0.5 : -0.5));
}

template<class T>
T stretch_range(T val, T old_min, T old_max, T new_min, T new_max){
	return ((val - old_min)/(old_max-old_min))*(new_max - new_min) + new_min;
}




template<class R=std::byte, class T>
std::span<R> image_as_blob(Storage::blob_id_t blob_id, const std::vector<T>& a, const PixelFormat input_pixel_format=GreyscalePixelFormat){
	GET_LOGGER;

	LOGV_DEBUG(blob_id, a, input_pixel_format.channels_per_pixel);

	T min = a[0], max=a[0];
	for(const T& item : a){
		if (item > max) max = item;
		if (item < min) min = item;
	}

	const PixelFormat output_pixel_format = RGBAPixelFormat;
	size_t size_of_image_data = round_to<int>((1.0*output_pixel_format.channels_per_pixel*a.size())/input_pixel_format.channels_per_pixel);

	Storage::blobs.try_emplace(
		blob_id,
		size_of_image_data
	);

	std::vector<std::byte>& image_data = Storage::blobs[blob_id];

	LOGV_DEBUG(image_data.size(), size_of_image_data);
	assert(image_data.size() == size_of_image_data);

	switch (input_pixel_format.id) {
		case PixelFormatId::GREYSCALE :{
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

		case PixelFormatId::RGBA :{
			for (size_t i=0; i < a.size(); ++i){
				image_data[i] = (std::byte)(round_to<uint8_t>(stretch_range(a[i], min, max, 0.0, 255.0)));
			}
			break;
		}

		default:{
			LOG_ERROR("Unknown pixel format id");
			std::exit(EXIT_FAILURE);
			break;
		}
	}

	return Storage::BlobMgr::get_as<R>(blob_id);
}


template<class T>
emscripten::val vector_as_JSImageData(Storage::blob_id_t blob_id, const std::vector<T>& a, const PixelFormat input_pixel_format=GreyscalePixelFormat){
	GET_LOGGER;
	LOGV_DEBUG(a.data());

	std::span<uint8_t> image_data = image_as_blob<uint8_t,T>(blob_id, a, input_pixel_format);
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

#endif //__JS_GLUE_INCLUDED__
