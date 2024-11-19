#include "tiff_helper.hpp"



TIFF_TileInfo::TIFF_TileInfo(TIFF* tptr){
	GET_LOGGER;
	n_tiles = TIFFNumberOfTiles(tptr);
	tile_size = TIFFTileSize(tptr);
	if (! TIFFGetField(tptr, TIFFTAG_TILELENGTH, &rows_per_tile)){
		LOG_ERROR("Could not determine rows_per_tile for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
}

TIFF_StripInfo::TIFF_StripInfo(TIFF* tptr){
	GET_LOGGER;
	n_strips = TIFFNumberOfStrips(tptr);
	strip_size = TIFFStripSize(tptr);
	if (! TIFFGetField(tptr, TIFFTAG_ROWSPERSTRIP, &rows_per_strip)){
		LOG_ERROR("Could not determine rows_per_strip for TIFF file. Exiting...");
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
	if (! TIFFGetField(tptr, TIFFTAG_PLANARCONFIG, &planar_config)){
		LOG_ERROR("Could not determine planar configuration for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	
	if (! TIFFGetField(tptr, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel)){
		LOG_WARN("Could not determine samples_per_pixel for TIFF file. Assuming 1");
		samples_per_pixel = 1;
	}
	
	uses_strips = TIFF_uses_strips(tptr);
	uses_tiles = TIFF_uses_tiles(tptr);
	
	bits_per_sample.resize(samples_per_pixel);
	sample_format.resize(samples_per_pixel);
	min_sample_value.resize(samples_per_pixel);
	max_sample_value.resize(samples_per_pixel);
	
	if (! TIFFGetField(tptr, TIFFTAG_BITSPERSAMPLE, bits_per_sample.data())){
		LOG_ERROR("Could not determine bits per sample for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	if (! TIFFGetField(tptr, TIFFTAG_SAMPLEFORMAT, sample_format.data())){
		LOG_ERROR("Could not determine sample_format for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	if (! TIFFGetField(tptr, TIFFTAG_SMINSAMPLEVALUE, min_sample_value.data())){
		LOG_WARN("Could not determine min_sample_value for TIFF file");
		for (int i=0; i< samples_per_pixel; ++i){
			min_sample_value[i] = 0;
		}
	}
	if (! TIFFGetField(tptr, TIFFTAG_SMAXSAMPLEVALUE, max_sample_value.data())){
		LOG_WARN("Could not determine max_sample_value for TIFF file");
		for (int i=0; i< samples_per_pixel; ++i){
			max_sample_value[i] = 1;
		}
	}
}


TIFF_ImageShape::TIFF_ImageShape(TIFF* tptr){
	width = TIFF_width(tptr);
	height = TIFF_height(tptr);
}

uint16_t TIFF_get_width(const std::string& name){
	GET_LOGGER;
	
	TIFF* tptr = TIFF_open(name, "rm");
	
	if (tptr == nullptr){
		LOG_ERROR("Could not open TIFF file %", name);
		exit(EXIT_FAILURE);
	}
	
	uint16_t image_width;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGEWIDTH, &image_width)){
		LOG_WARN("Could not determineimage_width for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	return image_width;
}

uint16_t TIFF_get_height(const std::string& name){
	GET_LOGGER;
	TIFF* tptr = TIFF_open(name, "rm");
	uint16_t ret;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGELENGTH, &ret)){
		LOG_WARN("Could not determine image length for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	return ret;
}

uint16_t TIFF_width(TIFF* tptr){
	GET_LOGGER;
	uint16_t image_width;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGEWIDTH, &image_width)){
		LOG_WARN("Could not determineimage_width for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}
	return image_width;
}

uint16_t TIFF_height(TIFF* tptr){
	GET_LOGGER;
	uint16_t ret;
	if (! TIFFGetField(tptr, TIFFTAG_IMAGELENGTH, &ret)){
		LOG_WARN("Could not determine image length for TIFF file. Exiting...");
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


std::vector<double> TIFF_read_strips(TIFF* tptr){
	// TIFFTAG_PLANARCONFIG = 1 then colour channels stored per pixel (i.e. each strip has {p1(RGB), p2(RGB),...}
	//                      = 2 then colour channels stored per strip (i.e. strip1={R R ...} strip2={G G ...} strip3={B B ...})
	GET_LOGGER;
	uint16_t planar_config;
	if (! TIFFGetField(tptr, TIFFTAG_PLANARCONFIG, &planar_config)){
		LOG_ERROR("Could not determine planar configuration for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	size_t n_strips = TIFFNumberOfStrips(tptr);
	size_t strip_size = TIFFStripSize(tptr);

	uint16_t bits_per_sample;
	if (! TIFFGetField(tptr, TIFFTAG_BITSPERSAMPLE, &bits_per_sample)){
		LOG_ERROR("Could not determine bits per sample for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	uint16_t samples_per_pixel;
	if (! TIFFGetField(tptr, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel)){
		LOG_WARN("Could not determine samples_per_pixel for TIFF file. Assuming 1");
		samples_per_pixel = 1;
	}

	uint16_t sample_format;
	if (! TIFFGetField(tptr, TIFFTAG_SAMPLEFORMAT, &sample_format)){
		LOG_ERROR("Could not determine sample_format for TIFF file. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	std::vector<double> data;

	switch (sample_format) {
		case 1:
			switch (bits_per_sample) {
				case 8:
					LOG_DEBUG("Reading Uint8");
					data = TIFF_read_strips_to_double<uint8_t>(tptr, n_strips, strip_size, planar_config, samples_per_pixel, 0.0, 255.0);
					break;
				case 16:
					LOG_DEBUG("Reading Uint16");
					data = TIFF_read_strips_to_double<uint16_t>(tptr, n_strips, strip_size, planar_config, samples_per_pixel, 0.0, 65535.0);
					break;
			}
			break;
		case 2:
			switch (bits_per_sample) {
				case 8:
					LOG_DEBUG("Reading Int8");
					data = TIFF_read_strips_to_double<int8_t>(tptr, n_strips, strip_size, planar_config, samples_per_pixel, -128., 127.);
					break;
				case 16:
					LOG_DEBUG("Reading Int16");
					data = TIFF_read_strips_to_double<int16_t>(tptr, n_strips, strip_size, planar_config, samples_per_pixel, -32768., 32767.);
					break;
			}
			break;
		case 3:
			switch (bits_per_sample){
				default:
					//data = TIFF_read_strips_to_double<double>(tptr, n_strips, strip_size, planar_config, samples_per_pixel);
					LOG_ERROR("Sample format 'double' for TIFF file not supported yet. Exiting...");
					std::exit(EXIT_FAILURE);
			}
			break;
		default:
			LOG_ERROR("Unknown sample format '%' for TIFF file. Exiting...", sample_format);
			std::exit(EXIT_FAILURE);
	}
	LOGV_DEBUG(data);
	return data;
}


// NOTE: Going to have to read the TIFF using TIFFReadEncodedStrip and/or TIFFReadEncodedTile
// as we cannot assume 8 bits per pixel and RGBA pixel layouts.
// May have to write our own class that sits on top of libtiff and intelligently finds the
// number of bytes per sample, samples per pixel, and dumps uncompressed raw image data into memory.

const std::string OPEN_FILE_TAG="__open_copy";

TIFF* TIFF_open(const std::string& name, const char* mode){
	GET_LOGGER;
	LOG_DEBUG("Opening TIFF file %", name);
	
	// ERROR: Calling TIFFClose(...) in any form invalidates the data we "stored on disk"
	// Therefore, need to always copy when opening
	
	std::vector<std::byte> persistent_data = Storage::BlobMgr::get(name);
	LOGV_DEBUG(&persistent_data);
	LOGV_DEBUG(persistent_data);
	
	Storage::BlobMgr::store_named(name+OPEN_FILE_TAG, persistent_data);
	
	std::vector<std::byte>& raw_data = Storage::BlobMgr::get(name+OPEN_FILE_TAG);
	
	LOGV_DEBUG(&raw_data);
	LOGV_DEBUG(raw_data);
	
	if (Storage::filelikes.contains(name)) {
		Storage::filelikes[name].bytes = std::span<std::byte>(raw_data.begin(), raw_data.end());
	}
	else {
		Storage::filelikes.emplace(
			name,
			FileLike(
				std::span<std::byte>(raw_data.begin(), raw_data.end())
			)
		);
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
		LOG_ERROR("Could not open TIFF file %", name);
		exit(EXIT_FAILURE);
	}
	
	if (MemFileRegistry.contains(name)){
		LOG_ERROR("TIFF file '%' is already open, cannot open twice", name);
	}
	
	MemFileRegistry[name] = FPtr(
		FileType::TIFF,
		tptr
	);
	
	LOGV_DEBUG(raw_data);
	LOGV_DEBUG(tiff_data->bytes);
	return tptr;
}

void TIFF_close(const std::string& name){
	GET_LOGGER;
	LOG_DEBUG("Closing TIFF file %", name);
	
	// ERROR: Calling TIFFClose(...) in any form invalidates the data we "stored on disk"
	// Therefore, need to always copy when opening
	
	
	
	if (MemFileRegistry.contains(name)){
		// Write the 'copy' to disk
		Storage::BlobMgr::store_named(name, Storage::BlobMgr::get(name+OPEN_FILE_TAG));
	
		LOG_DEBUG("MemFileRegistry contains file");
		TIFF* tptr = MemFileRegistry_get<TIFF>(name);
		LOG_DEBUG("Got pointer from MemFileRegistry");
		// File is closed when MemFileRegistry entry is deleted
		//TIFFClose(tptr);
		//LOG_DEBUG("Closed TIFF file via library function");
		MemFileRegistry_delete(name);
		
		LOG_DEBUG("MemFileRegistry entry deleted");
		//Storage::blobs.erase(Storage::blob_name_to_id_map[name+OPEN_FILE_TAG]);
		//LOG_DEBUG("Erased copied storage");
		//Storage::blob_name_to_id_map.erase(name+OPEN_FILE_TAG);
		//LOG_DEBUG("Erased name to id mapping");
	}
	LOG_DEBUG("TIFF file closed");
}

std::string TIFF_from_js_array(const std::string& name, const emscripten::val& uint8_array){
	GET_LOGGER;

	// Store the raw TIFF data in a blob so we can access it later
	if (!Storage::BlobMgr::has(name)){
		Storage::BlobMgr::store_named(name, emscripten::convertJSArrayToNumberVector<uint8_t>(uint8_array));
	}
	
	std::vector<std::byte>* raw_data_ptr;
	
	TIFF* tptr = TIFF_open(name, "rm");
	raw_data_ptr = &(Storage::BlobMgr::get(name));
	LOGV_DEBUG(raw_data_ptr);
	LOGV_DEBUG(*raw_data_ptr);
	
	TIFF_close(name); // ERROR: For some reason this changes the stored data
	raw_data_ptr = &(Storage::BlobMgr::get(name));
	LOGV_DEBUG(raw_data_ptr);
	LOGV_DEBUG(*raw_data_ptr);
	
	tptr = TIFF_open(name, "rm");
	raw_data_ptr = &(Storage::BlobMgr::get(name));
	LOGV_DEBUG(raw_data_ptr);
	LOGV_DEBUG(*raw_data_ptr);
	
	

	LOG_DEBUG("File % read in successfully", name);

	TIFF_dump_tags(tptr);

	if (!TIFF_uses_strips(tptr)){
		LOG_ERROR("Only TIFF files that use strips are supported at the moment. Exiting...");
		std::exit(EXIT_FAILURE);
	}

	Storage::images.emplace(std::make_pair(
		name, 
		Image(
			std::vector<size_t>({TIFF_width(tptr), TIFF_height(tptr)}),
			TIFF_read_strips(tptr),
			GreyscalePixelFormat // Hard-coded for now	
		)
	));

	TIFF_close(name);

	LOG_DEBUG("Stored image using name '%'", name);

	return name;
}


