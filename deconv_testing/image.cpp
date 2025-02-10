#include "image.hpp"

// Internal image format is ALWAYS {R,R,R, ..., G,G,G, ..., B,B,B,...}
const PixelFormat GreyscalePixelFormat(PixelFormatId::GREYSCALE, 1, {ChannelName::BRIGHTNESS});
const PixelFormat RGBPixelFormat(PixelFormatId::RGB, 3, {ChannelName::RED, ChannelName::GREEN, ChannelName::BLUE});
const PixelFormat RGBAPixelFormat(PixelFormatId::RGBA, 4, {ChannelName::RED, ChannelName::GREEN, ChannelName::BLUE, ChannelName::ALPHA});

