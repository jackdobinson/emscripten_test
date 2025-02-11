#include "image.hpp"

// Internal image format is ALWAYS {R,R,R, ..., G,G,G, ..., B,B,B,...}
const PixelFormat UndefinedPixelFormat(PixelFormatId::UNDEFINED, 1, {ChannelName::UNDEFINED});
const PixelFormat GreyscalePixelFormat(PixelFormatId::GREYSCALE, 1, {ChannelName::BRIGHTNESS});
const PixelFormat RGBPixelFormat(PixelFormatId::RGB, 3, {ChannelName::RED, ChannelName::GREEN, ChannelName::BLUE});
const PixelFormat RGBAPixelFormat(PixelFormatId::RGBA, 4, {ChannelName::RED, ChannelName::GREEN, ChannelName::BLUE, ChannelName::ALPHA});


Image::Image():
	write_head_idx(0),
	read_head_idx(0),
	shape(0),
	data(0),
	pxfmt(UndefinedPixelFormat)
{}

Image::Image(const std::vector<size_t>& _shape, const PixelFormat& _pxfmt):
	write_head_idx(0),
	read_head_idx(0),
	shape(_shape),
	data(du::product(_shape)),
	pxfmt(_pxfmt)
{}