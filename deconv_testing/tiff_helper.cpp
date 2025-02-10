#include "tiff_helper.hpp"



TIFF_TileInfo::TIFF_TileInfo(TIFF* tptr){
	GET_LOGGER;
	n_tiles = TIFFNumberOfTiles(tptr);
	tile_size = TIFFTileSize(tptr);
	if (! TIFFGetField(tptr, TIFFTAG_TILELENGTH, &rows_per_tile)){
		puts("Error: Could not determine rows_per_tile for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
}

TIFF_StripInfo::TIFF_StripInfo(TIFF* tptr){
	GET_LOGGER;
	n_strips = TIFFNumberOfStrips(tptr);
	strip_size = TIFFStripSize(tptr);
	if (! TIFFGetField(tptr, TIFFTAG_ROWSPERSTRIP, &rows_per_strip)){
		puts("Error: Could not determine rows_per_strip for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
}

void TIFF_LayoutInfo::fill_from(const TIFF_StripInfo& other){
	n_items = other.n_strips;
	item_size = other.strip_size;
	rows_per_item = other.rows_per_strip;
}
void TIFF_LayoutInfo::fill_from(const TIFF_TileInfo& other){
	n_items = other.n_tiles;
	item_size = other.tile_size;
	rows_per_item = other.rows_per_tile;
}

TIFF_PixelInfo::TIFF_PixelInfo(TIFF* tptr){
	GET_LOGGER;
	
	if (! TIFFGetField(tptr, TIFFTAG_PHOTOMETRIC, &photometric_interpretation)){
		puts("Error: Could not determine photometric interpretation for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	
	if (! TIFFGetField(tptr, TIFFTAG_PLANARCONFIG, &planar_config)){
		puts("Warning: Could not determine planar configuration for TIFF file. Assuming 1 (Chunky format)");
		planar_config = 1;
	}
	
	if (! TIFFGetField(tptr, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel)){
		puts("Warning: Could not determine samples_per_pixel for TIFF file. Assuming 1");
		samples_per_pixel = 1;
	}
	
	uses_strips = TIFF_uses_strips(tptr);
	uses_tiles = TIFF_uses_tiles(tptr);
	
	bits_per_sample.resize(samples_per_pixel);
	sample_format.resize(samples_per_pixel);
	min_sample_value.resize(samples_per_pixel);
	max_sample_value.resize(samples_per_pixel);
	
	if (! TIFFGetField(tptr, TIFFTAG_BITSPERSAMPLE, bits_per_sample.data())){
		puts("Warning: Could not determine bits per sample for TIFF file. Assuming 1");
		for(auto& item : bits_per_sample){
			item = 1;
		}
	}
	if (! TIFFGetField(tptr, TIFFTAG_SAMPLEFORMAT, sample_format.data())){
		puts("Warning: Could not determine sample_format for TIFF file. Assuming 1 (unsigned integer)");
		for(auto& item : sample_format){
			item = 1;
		}
	}
	if (! TIFFGetField(tptr, TIFFTAG_SMINSAMPLEVALUE, min_sample_value.data())){
		puts("Warning: Could not determine min_sample_value for TIFF file, setting to 0");
		for (int i=0; i< samples_per_pixel; ++i){
			min_sample_value[i] = 0;
		}
	}
	if (! TIFFGetField(tptr, TIFFTAG_SMAXSAMPLEVALUE, max_sample_value.data())){
		puts("Warning: Could not determine max_sample_value for TIFF file, setting to 1");
		for (int i=0; i< samples_per_pixel; ++i){
			max_sample_value[i] = 1;
		}
	}
}


TIFF_ImageShape::TIFF_ImageShape(TIFF* tptr){
	width = TIFF_width(tptr);
	height = TIFF_height(tptr);
}

TIFF_Ptr::TIFF_Ptr(const std::string& _name, const char* mode){
	GET_LOGGER;
	name = _name;
	p = nullptr;
	should_close=false;
	
	auto tptr_it = Storage::objects<TIFF*>.find(name);
	if(tptr_it == Storage::objects<TIFF*>.end()){
		std::cerr << "Error: Could not get TIFF pointer as no TIFF pointer for " << name << std::endl;
	} else {
		if(tptr_it->second == nullptr){
			std::cerr << "Could not get TIFF pointer, as TIFF pointer if NULL for " << name << std::endl;
		} else {
			LOG_DEBUG("Got TIFF pointer without opening file, therefore should NOT close it after I am done");
			p = tptr_it->second;
		}
	}
	
	if (p == nullptr){
		p = TIFF_open(name, mode);
		should_close=true;
		LOG_DEBUG("Got TIFF pointer but had to open file, therefore should close it after I am done");
	}
}
TIFF_Ptr::~TIFF_Ptr(){
	if(should_close){
		TIFF_close(name);
	}
}

TIFF_Ptr::operator TIFF*() const {
	return p;
}

uint32_t TIFF_get_width(const std::string& name){
	GET_LOGGER;
	LOG_DEBUG("Getting width of TIFF file '%'", name);
	TIFF_Ptr tptr(name);
	
	uint32_t image_width = TIFF_width(tptr);
	
	LOG_DEBUG("Returning width of image: %", image_width);
	return image_width;
}

uint32_t TIFF_get_height(const std::string& name){
	GET_LOGGER;
	LOG_DEBUG("Getting height of TIFF file '%'", name);
	TIFF_Ptr tptr(name);
	
	uint32_t height = TIFF_height(tptr);
	
	LOG_DEBUG("Returning height of image: %", height);
	return height;
}

uint32_t TIFF_width(TIFF* tptr){
	uint32_t image_width;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGEWIDTH, &image_width)){
		puts("Error: Could not determine image_width for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	return image_width;
}

uint32_t TIFF_height(TIFF* tptr){
	uint32_t ret;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGELENGTH, &ret)){
		puts("Error: Could not determine image_length for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	return ret;
}

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


std::vector<double> TIFF_read_strips(TIFF* tptr, TIFF_PixelInfo& tiff_pixel_info){
	// TIFFTAG_PLANARCONFIG = 1 then colour channels stored per pixel (i.e., each strip has {p1(RGB), p2(RGB),...}
	//                      = 2 then colour channels stored per strip (i.e., strip1={R R ...} strip2={G G ...} strip3={B B ...})
	GET_LOGGER;
	TIFF_StripInfo tiff_strip_info(tptr);
	
	// Throw out error messages for unsupported formats
	if (tiff_pixel_info.samples_per_pixel > 1){
		std::cerr << "Error: More than one sample per pixel not supported yet. " << tiff_pixel_info.samples_per_pixel << " > 1. Exiting..." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	
	if(	(tiff_pixel_info.bits_per_sample[0] != 8)
		|| (tiff_pixel_info.bits_per_sample[0] != 16)
	){
		puts("Error: Only 8 or 16 bits per sample currently supported");
		std::exit(EXIT_FAILURE);
	}
	

	std::vector<double> data;

	switch (tiff_pixel_info.sample_format[0]) {
		case 1:
			switch (tiff_pixel_info.bits_per_sample[0]) {
				case 8:
					LOG_DEBUG("Reading Uint8");
					data = TIFF_read_strips_to_double<uint8_t>(tptr, tiff_strip_info.n_strips, tiff_strip_info.strip_size, tiff_pixel_info.planar_config, tiff_pixel_info.samples_per_pixel, 0.0, 255.0);
					break;
				case 16:
					LOG_DEBUG("Reading Uint16");
					data = TIFF_read_strips_to_double<uint16_t>(tptr, tiff_strip_info.n_strips, tiff_strip_info.strip_size, tiff_pixel_info.planar_config, tiff_pixel_info.samples_per_pixel, 0.0, 65535.0);
					break;
			}
			break;
		case 2:
			switch (tiff_pixel_info.bits_per_sample[0]) {
				case 8:
					LOG_DEBUG("Reading Int8");
					data = TIFF_read_strips_to_double<int8_t>(tptr, tiff_strip_info.n_strips, tiff_strip_info.strip_size, tiff_pixel_info.planar_config, tiff_pixel_info.samples_per_pixel, -128., 127.);
					break;
				case 16:
					LOG_DEBUG("Reading Int16");
					data = TIFF_read_strips_to_double<int16_t>(tptr, tiff_strip_info.n_strips, tiff_strip_info.strip_size, tiff_pixel_info.planar_config, tiff_pixel_info.samples_per_pixel, -32768., 32767.);
					break;
			}
			break;
		case 3:
			switch (tiff_pixel_info.bits_per_sample[0]){
				default:
					//data = TIFF_read_strips_to_double<double>(tptr, n_strips, strip_size, planar_config, samples_per_pixel);
					puts("Error: Sample format 'double' for TIFF file not supported yet. Exiting...");
					std::exit(EXIT_FAILURE);
			}
			break;
		default:
			std::cerr << "Error: Unsupported sample format " << tiff_pixel_info.sample_format[0] << " for TIFF file. Exiting..." << std::endl;
			std::exit(EXIT_FAILURE);
	}
	LOGV_DEBUG(data);
	return data;
}


// NOTE: Going to have to read the TIFF using TIFFReadEncodedStrip and/or TIFFReadEncodedTile
// as we cannot assume 8 bits per pixel and RGBA pixel layouts.
// May have to write our own class that sits on top of libtiff and intelligently finds the
// number of bytes per sample, samples per pixel, and dumps uncompressed raw image data into memory.



TIFF* TIFF_open(const std::string& name, const char* mode){
	GET_LOGGER;
	LOG_DEBUG("Opening TIFF file '%'", name);
	
	//const std::string opened_file_name = Storage::get_opened_file_name(name);
	
	// ERROR: Calling TIFFClose(...) in any form invalidates the data we "stored on disk"
	// Therefore, need to always copy when opening
	
	std::vector<std::byte>& persistent_data = Storage::named_blobs.at(name);
	LOGV_DEBUG(&persistent_data);
	LOGV_DEBUG(persistent_data);

	LOG_DEBUG("Printing FileLike map:");
	for(const auto& [key, value] : Storage::filelikes){
		LOG_DEBUG("\t% : file_name % bytes % pos % is_open % contents_changed %", key, value.file_name, value.bytes, value.pos, value.is_open, value.contents_changed);
	}

	
	auto filelike_it = Storage::filelikes.find(name);
	if (filelike_it == Storage::filelikes.end()) { // not present, so create it
		LOG_DEBUG("Not found FileLike for '%', creating a new one...", name);
		Storage::filelikes[name] = FileLike(name, persistent_data);
	}
	else { // is present so copy data to it
		LOG_DEBUG("Found FileLike for '%', reusing...", name);
		filelike_it->second.set_open_with_contents(persistent_data);
	}
	
	FileLike* tiff_data = &(Storage::filelikes[name]);
	LOGV_DEBUG(tiff_data);
	LOGV_DEBUG(tiff_data->bytes);
	
	TIFF* tptr = TIFFClientOpen(
		name.c_str(), 
		mode, // r - read, m - don't memory map file
		(thandle_t)(tiff_data),
		FileLike::readproc,
		FileLike::writeproc,
		FileLike::seekproc,
		FileLike::closeproc,
		FileLike::sizeproc,
		FileLike::mapfileproc,
		FileLike::unmapfileproc
	);
	
	LOGV_DEBUG(tptr);
	
	if (tptr == nullptr){
		std::cerr << "Could not open TIFF file " << name << std::endl;
		exit(EXIT_FAILURE);
	}
	
	Storage::objects<TIFF*>[name] = tptr;
	
	LOGV_DEBUG(tiff_data->bytes);
	return tptr;
}

void TIFF_close(const std::string& name){
	GET_LOGGER;
	LOG_DEBUG("Closing TIFF file '%'", name);
	
	// ERROR: Calling TIFFClose(...) in any form invalidates the data we "stored on disk"
	// Therefore, need to always copy when opening
	
	auto tptr_it = Storage::objects<TIFF*>.find(name);
	
	// If we have a valid pointer to a TIFF object under the passed name,
	// we should close the file. Otherwise we throw an error
	
	if(tptr_it == Storage::objects<TIFF*>.end()){
		std::cerr << "Could not find TIFF* for file " <<  name <<std::endl;
		exit(EXIT_FAILURE);
	}
	if ((tptr_it->second) == nullptr){
		std::cerr << "TIFF* for file is null, cannot close a null pointer for file " << name << std::endl;
		exit(EXIT_FAILURE);
	}
	
	auto filelike_it = Storage::filelikes.find(name);
	if(filelike_it == Storage::filelikes.end()){
		std::cerr << "Could not find FileLike for file " << name << std::endl;
		exit(EXIT_FAILURE);
	}
	if(!(filelike_it->second.is_open)){
		std::cerr << "FileLike for file is not open, cannot close a non-open file " << name << std::endl;
		exit(EXIT_FAILURE);
	}
	
	if (filelike_it->second.contents_changed){
		LOG_DEBUG("Contents of filelike '%' have changed, writing to persistent storage...", filelike_it->second.file_name);
		Storage::named_blobs[filelike_it->second.file_name] = filelike_it->second.bytes;
	}
	
	TIFFClose(tptr_it->second);
	tptr_it->second = nullptr; // set to null after closing
	
	LOG_DEBUG("TIFF file closed");
}

std::string TIFF_from_js_array(const std::string& name, const emscripten::val& uint8_array){
	GET_LOGGER;

	// Store the raw TIFF data in a blob so we can access it later
	if (!Storage::named_blobs.contains(name)){
		const std::vector<uint8_t>& temp = emscripten::convertJSArrayToNumberVector<uint8_t>(uint8_array);
		Storage::named_blobs.emplace(
			name, 
			std::vector<std::byte>((const std::byte*)(temp.data()), (const std::byte*)(temp.data()+temp.size()))
		);
	}
	
	std::vector<std::byte>* raw_data_ptr;
	
	LOG_DEBUG("Open Once");
	TIFF* tptr = TIFF_open(name, "rm");
	raw_data_ptr = &(Storage::named_blobs.at(name));
	LOGV_DEBUG(raw_data_ptr);
	LOGV_DEBUG(*raw_data_ptr);
	
	//puts("Close");
	//TIFF_close(name); // ERROR: For some reason this changes the stored data
	//raw_data_ptr = &(Storage::named_blobs.at(name));
	LOGV_DEBUG(raw_data_ptr);
	LOGV_DEBUG(*raw_data_ptr);
	
	//puts("Open Again");
	//tptr = TIFF_open(name, "rm");
	//raw_data_ptr = &(Storage::named_blobs.at(name));
	LOGV_DEBUG(raw_data_ptr);
	LOGV_DEBUG(*raw_data_ptr);
	
	TIFF_PixelInfo tiff_pixel_info(tptr);

	LOG_DEBUG("File % read in successfully", name);
	
	//TIFF_dump_tags(tptr);

	//if (!TIFF_uses_strips(tptr)){
	if (!tiff_pixel_info.uses_strips) {
		puts("Only TIFF files that use strips are supported at the moment. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	
	const PixelFormat* image_px_format = nullptr;
	if (tiff_pixel_info.samples_per_pixel == 1){
		image_px_format = &GreyscalePixelFormat;
	} else if (tiff_pixel_info.samples_per_pixel == 3){
		image_px_format = &RGBPixelFormat;
	} else if (tiff_pixel_info.samples_per_pixel == 4) {
		image_px_format = &RGBAPixelFormat;
	} else {
		puts("Error: Could not map samples per pixel to internal pixel format. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	Storage::images.emplace(
		std::make_pair(
			name, 
			Image(
				std::vector<size_t>({TIFF_width(tptr), TIFF_height(tptr), tiff_pixel_info.samples_per_pixel}), // x, y, z
				TIFF_read_strips(tptr, tiff_pixel_info),
				std::cref(*image_px_format)
			)
		)
	);

	TIFF_close(name);

	LOG_DEBUG("Stored image using name '%'", name);
	return name;
}


