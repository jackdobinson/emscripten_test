#include "main.h"


using namespace emscripten;


using js_ptr = int;

val get_byte_buffer(js_ptr ptr, size_t size){
	return val(typed_memory_view(size,(uint8_t*)ptr));
}

js_ptr alloc(size_t size){
	return (js_ptr)malloc(size);
}



void remove_image(const std::string& name){
	Storage::images.erase(name);
}


emscripten::val image_as_JSImageData(const std::string& name){
	GET_LOGGER;
	LOGV_DEBUG(name);

	const Image& image = Storage::images[name];

	LOGV_DEBUG(image.data);

	std::span<uint8_t> image_data = image_as_blob<uint8_t>(name, image.data, image.pxfmt);


	return emscripten::val(emscripten::typed_memory_view(image_data.size(), image_data.data()));

}


void DUMMY_load_data(std::string fname, std::vector<double>& img_data, std::vector<int>& img_shape){
	GET_LOGGER;

	if (fname=="image") {
		img_data = {	
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
		};

		img_shape={20,30};
		return;
	}

	if (fname=="psf"){
		/*
		;img_data = {	
			0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,
			0,1,1,1,1,1,0,
			0,1,1,1,1,1,0,
			0,1,1,1,1,1,0,
			0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,
		}
		*/
		img_data = {	
			0,0,0,0,0,0,0,
			0,0,1,1,1,0,0,
			0,1,1,1,1,1,0,
			0,0,1,1,1,0,0,
			0,0,0,0,0,0,0,
		};

		img_shape={7,5};
		return;
	}

	LOG_ERROR("Dummy data can only supply 'image' and 'psf'");

}



std::vector<std::string> deconv_types = {"clean_modified"};
std::map<std::string, CleanModifiedAlgorithm> clean_modified_deconvolvers;
std::string current_deconv_type = "";
std::string current_deconv_name = "";



int create_deconvolver(const std::string& deconv_type, const std::string& deconv_name){//, int max_n_iters){
	GET_LOGGER;

	// Ensure we have good arguments
	if(! du::contains(deconv_types, deconv_type)){
		LOG_MSG_START("Passed '%' as argument deconv_type. Must be one of the following {");
		for(auto& item : deconv_type){
			LOG_MSG_CONT("%, ", item);
		}
		LOG_MSG_CONT("}");
		LOG_ERROR("%", LOG_MSG_GET);
		return -1;
	}

	// remove previous deconvolver
	if (current_deconv_name.size() != 0){
		clean_modified_deconvolvers.erase(deconv_name);
	}

	// initialise deconvolver, choose based on 'deconv_type'
	
	current_deconv_type = deconv_type;
	current_deconv_name = deconv_name;

	// Only have one deconvolver type for now, so use that one
	clean_modified_deconvolvers[deconv_name] = CleanModifiedAlgorithm();

	return 0;
}


void prepare_deconvolver(
		const std::string& deconv_type, 
		const std::string& deconv_name, 
		const std::string& sci_image_name, 
		const std::string& psf_image_name, 
		const std::string& run_tag=""
	){
	GET_LOGGER;
	// get deconvolver
	// Only one type for now, so use that
	
	CleanModifiedAlgorithm& deconvolver = clean_modified_deconvolvers[deconv_name];

	Image& sci_image = Storage::images[sci_image_name];
	Image& psf_image = Storage::images[psf_image_name];

	deconvolver.prepare_observations(
		sci_image.data,
		sci_image.shape,
		psf_image.data,
		psf_image.shape,
		run_tag
	);
}

void run_deconvolver(
		const std::string& deconv_type, 
		const std::string& deconv_name
	){
	GET_LOGGER;
	// get deconvolver
	// Only one type for now, so use that
	
	CleanModifiedAlgorithm& deconvolver = clean_modified_deconvolvers[deconv_name];
	
	deconvolver.run();

	//const std::span<std::byte>& temp = image_as_blob(deconvolver.BLOB_ID_CLEAN_MAP, deconvolver.clean_map);
	//LOG_DEBUG("sending clean_map to debug canvas");
	//send_to_js_canvas(temp.data(), temp.size(), sci_image.shape[0]+1-sci_image.shape[0]%2, sci_image.shape[1]+1-sci_image.shape[1]%2);

}


emscripten::val get_deconvolver_clean_map(
		const std::string& deconv_type,
		const std::string& deconv_name
	){
	GET_LOGGER;
	LOG_DEBUG("Sending clean map");
	CleanModifiedAlgorithm& deconvolver = clean_modified_deconvolvers[deconv_name];

	return vector_as_JSImageData(deconvolver.BLOB_ID_CLEAN_MAP, deconvolver.clean_map);
}

emscripten::val get_deconvolver_residual(
		const std::string& deconv_type,
		const std::string& deconv_name
	){
	GET_LOGGER;
	LOG_DEBUG("Sending residual data");

	CleanModifiedAlgorithm& deconvolver = clean_modified_deconvolvers[deconv_name];

	return vector_as_JSImageData(deconvolver.BLOB_ID_RESIDUAL, deconvolver.residual_data);
}

// TODO: 
// * When outputting in TIFF format, how to I preserve the flux-conservation properties of
//   the deconvolution algorithm?
emscripten::val get_tiff(
		const std::string& deconv_type, 
		const std::string& deconv_name, 
		const std::string& file_id,
		const std::string& original_file_name
	){
	GET_LOGGER;
	// Only one deconvolver type right now, therefore skip deconv_type dispatch
	const CleanModifiedAlgorithm& deconv = clean_modified_deconvolvers[deconv_name];
	LOGV_DEBUG(deconv.data_shape, deconv.data_shape_adjustment);
	std::vector<size_t> raw_data_shape = du::subtract(deconv.data_shape, deconv.data_shape_adjustment);
	std::vector<double> raw_data(du::product(raw_data_shape));
	
	if (file_id == "deconv.clean_map"){
		raw_data = du::reshape(deconv.clean_map, deconv.data_shape, raw_data_shape); 
	}
	if (file_id == "deconv.residual"){
		raw_data = du::reshape(deconv.residual_data, deconv.data_shape, raw_data_shape); 
	}

	const std::span<uint8_t>& tiff_data = TIFF_bytes_like(original_file_name, original_file_name + file_id, raw_data);
	
	return emscripten::val(emscripten::typed_memory_view(tiff_data.size(), tiff_data.data()));

}



int main(int argc, char** argv){
	INIT_LOGGING("DEBUG");
	GET_LOGGER;
	
	// Return here if we don't want to run the test bits
	return 0;

	
	// Use this to test the routines
	//assert(argc == 3);

	//std::string input_fpath(argv[1]);
	//std::string PSF_fpath(argv[2]);

	std::string input_fpath="image";
	std::string PSF_fpath="psf";

	
	LOGV_DEBUG(input_fpath);
	std::vector<double> img_data;
	std::vector<int> img_shape;
	DUMMY_load_data(input_fpath, img_data, img_shape);
	LOGV_DEBUG(img_data.size());
	LOGV_DEBUG(img_shape);

	LOGV_DEBUG(PSF_fpath);
	std::vector<double> psf_data;
	std::vector<int> psf_shape;
	DUMMY_load_data(PSF_fpath, psf_data, psf_shape);
	LOGV_DEBUG(psf_data.size());
	LOGV_DEBUG(psf_shape);

	emscripten_sleep(100);
	
	//du::write_as_image("./obs_img.pgm", img_data, img_shape);
	//du::write_as_image("./psf_img.pgm", psf_data, psf_shape);

	LOG_DEBUG("Creating deconvolver");
	CleanModifiedAlgorithm deconv_algorithm(100);
	
	emscripten_sleep(100);

	LOG_DEBUG("Printing deconvolver parameters");
	deconv_algorithm.__str__();

	emscripten_sleep(100);

	LOG_DEBUG("Running deconvolver");
	deconv_algorithm.prepare_observations(
		img_data,
		du::as_type<size_t>(img_shape), 
		psf_data, 
		du::as_type<size_t>(psf_shape)
	);
	
	deconv_algorithm.run();
	


}

void set_deconvolver_parameters(
		const std::string& deconv_type,
		const std::string& deconv_name,
		size_t _n_iter,
		size_t _n_positive_iter,
		double _loop_gain,
		bool _adaptive_threshold,
		double _threshold,
		double _clean_beam_gaussian_sigma,
		bool _add_residual,
		double _noise_std,
		double _rms_frac_threshold,
		double _fabs_frac_threshold
	){
	CleanModifiedAlgorithm& deconvolver = clean_modified_deconvolvers[deconv_name];
	
	deconvolver.n_iter= _n_iter;
	deconvolver.n_positive_iter = _n_positive_iter;
	deconvolver.loop_gain = _loop_gain;
	if (_adaptive_threshold){
		deconvolver.threshold = -1;
	}else{
		deconvolver.threshold = _threshold;
	}
	deconvolver.clean_beam_gaussian_sigma = _clean_beam_gaussian_sigma;
	deconvolver.add_residual = _add_residual;
	deconvolver.noise_std = _noise_std;
	deconvolver.rms_frac_threshold = _rms_frac_threshold;
	deconvolver.fabs_frac_threshold = _fabs_frac_threshold;
	
	// Update other deconvolver attributes that depend on these params
	deconvolver.fabs_record.resize(_n_iter, NAN); 
	deconvolver.rms_record.resize(_n_iter, NAN);
	deconvolver.threshold_record.resize(_n_iter, NAN); 
}


EMSCRIPTEN_BINDINGS(my_module){
	function("get_tiff", &get_tiff);
	function("create_deconvolver", &create_deconvolver);
	function("prepare_deconvolver", &prepare_deconvolver);
	function("run_deconvolver", &run_deconvolver);
	function("get_deconvolver_clean_map", &get_deconvolver_clean_map);
	function("get_deconvolver_residual", &get_deconvolver_residual);
	function("remove_image", &remove_image);
	function("TIFF_get_width", &TIFF_get_width);
	function("TIFF_get_height", &TIFF_get_height);
	function("image_as_JSImageData", &image_as_JSImageData);
	function("TIFF_from_js_array", &TIFF_from_js_array);
	function("get_byte_buffer", &get_byte_buffer);//, return_value_policy::reference());
	function("alloc",&alloc);
	
	function("set_deconvolver_parameters",&set_deconvolver_parameters);

};

