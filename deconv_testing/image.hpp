#ifndef __IMAGE_INCLUDED__
#define __IMAGE_INCLUDED__

#include <vector>



enum class PixelFormatId{
	GREYSCALE,
	RGBA
};

enum class ChannelName{
	RED,
	GREEN,
	BLUE,
	ALPHA,
	BRIGHTNESS
};

struct PixelFormat{
	PixelFormatId id;
	size_t channels_per_pixel;
	std::vector<ChannelName> channel_names;
};


const PixelFormat GreyscalePixelFormat(PixelFormatId::GREYSCALE, 1, {ChannelName::BRIGHTNESS});
const PixelFormat RGBAPixelFormat(PixelFormatId::GREYSCALE, 4, {ChannelName::RED, ChannelName::GREEN, ChannelName::BLUE, ChannelName::ALPHA});


struct Image{
	std::vector<size_t> shape;
	std::vector<double> data;
	PixelFormat pxfmt;
};

#endif //__IMAGE_INCLUDED__


