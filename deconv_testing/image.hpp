#ifndef __IMAGE_INCLUDED__
#define __IMAGE_INCLUDED__

#include <vector>



enum class PixelFormatId{
	GREYSCALE,
	RGB,
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

// Internal image format is ALWAYS {R,R,R, ..., G,G,G, ..., B,B,B,...}
extern const PixelFormat GreyscalePixelFormat;
extern const PixelFormat RGBPixelFormat;
extern const PixelFormat RGBAPixelFormat;


struct Image{
	std::vector<size_t> shape; // x, y, z
	std::vector<double> data; // XXX... | RRR...GGG...BBB... | RRR...GGG...BBB...AAA...
	PixelFormat pxfmt;
};

#endif //__IMAGE_INCLUDED__


