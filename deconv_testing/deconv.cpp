#include "deconv.hpp"


using namespace emscripten;

/*
void load_arbitrary_fits_data(std::string fname, std::vector<double>& img_data, std::vector<int>& img_shape){
	// At the moment, only ouputs 2-dimensional data. If 3D data is supplied will take the middle of the 3rd axis.
	GET_LOGGER;
	fitsfile *fptr;
	int status=0;
	int ndim=0, bitpix=0;
	long npixels=0;

	LOGV_DEBUG(fname);

	fits_open_file(&fptr, fname.c_str(), READONLY, &status);

	fits_get_img_equivtype(fptr, &bitpix, &status);
	fits_get_img_dim(fptr, &ndim, &status);
	LOGV_DEBUG(bitpix, ndim);

	std::vector<long> naxes(ndim,-1);

	fits_get_img_size(fptr, ndim, naxes.data(), &status);
	for(int i=0; i< ndim; ++i){
		LOG_DEBUG("naxes[%] %", i, naxes[i]);
	}

	npixels = du::product(naxes);
	LOGV_DEBUG(npixels);


	std::vector<long> fpixel(ndim, 1);
	
	//fpixel[2] = naxes[2]/2;
	//for(int i=0; i<ndim; ++i) {
	//	LOGV_DEBUG(fpixel[i]);
	//}
	//
	//long nread_pixels = naxes[0]*naxes[1];
	//
	//LOGV_DEBUG(nread_pixels);
	//
	//img_data.resize(nread_pixels);
	//img_shape.resize(2);
	//img_shape[0] = naxes[0];
	//img_shape[1] = naxes[1];
	

	long nread_pixels = du::product(naxes);
	img_data.resize(nread_pixels);

	img_shape.resize(ndim);
	for(int i=0; i< ndim; i++){
		img_shape[i] = naxes[i];
	}

	std::vector<int> img_null(nread_pixels,0);

	fits_read_pix(fptr, TDOUBLE, fpixel.data(), nread_pixels, 0, img_data.data(), img_null.data(), &status);

	fits_close_file(fptr, &status);

	return;
}
*/

// TODO:
// * Move into it's own file
// * Can I make this more generic, i.e. can I write to a named canvas?
// * Can I expose a set of "data sources" from CleanModifiedAlgorithm, create
//   a load of these functions as "data representers", then associate "data sources"
//   with "data representers" on the JS side? I.e. C++ give JS a handle to some raw
//   data. JS (or here) has functions that display raw data. I associate raw data
//   handles with display functions on JS side. C++ side updates raw data, JS side
//   periodically calls display functions.
EM_JS(void, send_to_js_canvas, (void* ptr, int size, int width, int height), {
	console.log("TESTING EM_ASM");
	console.log(ptr, size, width, height);
	let im_data = new ImageData(new Uint8ClampedArray(Module.HEAPU8.buffer, ptr, size), width, height);
	scratch_canvas.width = width;
	scratch_canvas.height = height;
	scratch_canvas_ctx.imageSmoothingEnabled = false;
	scratch_canvas_ctx.putImageData(im_data, 0, 0);
});


// TODO: 
// * rewrite this to be easier to integrate with the JS side of things
// * On the JS side, need to be able to get handles for any plots. So they need to be exposed here.
// * The C++ side needs to be able to accept values from the JS interface to set algorithm parameters.
// * Break up ".run()" method into smaller chunks. Ideally want to have a ".doIter()" method that does
//   a single iteration.
// * Should pre-allocate as much as possible. Ideally will be able to give JS side a chunk of memory
//   where in-progress plot data will be written and have it update the plots in a loop.
// * May need to run the C++ side in a web-worker.
CleanModifiedAlgorithm::CleanModifiedAlgorithm(
		size_t _n_iter,
		size_t _n_positive_iter,
		double _loop_gain,
		double _threshold,
		double _clean_beam_gaussian_sigma,
		bool _add_residual,
		double _noise_std,
		double _rms_frac_threshold,
		double _fabs_frac_threshold
	): n_iter(_n_iter), n_positive_iter(_n_positive_iter), loop_gain(_loop_gain),
	threshold(_threshold), clean_beam_gaussian_sigma(_clean_beam_gaussian_sigma), add_residual(_add_residual),
	noise_std(_noise_std), rms_frac_threshold(_rms_frac_threshold), fabs_frac_threshold(_fabs_frac_threshold),
	residual_data(0), components_data(0), px_choice_map(0), padded_psf_data(0), selected_pixels(0), current_convolved(0),
	fabs_record(_n_iter), rms_record(_n_iter), fft(), ifft(), psf_fft(), selected_px_fft(), tag("")
{
}

void CleanModifiedAlgorithm::_get_residual_from_obs(const std::vector<double>& obs_data, const std::vector<size_t>& obs_shape){
	GET_LOGGER;
	std::vector<bool> obs_nan_mask(obs_data.size());
	residual_data = obs_data;
	LOGV_DEBUG(obs_data.size(), obs_nan_mask.size(), residual_data.size());

	obs_nan_mask = du::mask_where(residual_data, std::function<bool(double)>(du::isnan<double>));
	du::set_at_mask(residual_data, obs_nan_mask, 0.0);
}

void CleanModifiedAlgorithm::_get_padded_psf(const std::vector<double>& psf_data, const std::vector<size_t>& obs_shape, const std::vector<size_t>& psf_shape){
	GET_LOGGER;

	// zero the array that will hold our result
	du::multiply_inplace(padded_psf_data, 0);

	// define the first pixel that psf_data will be written from 
	std::vector<size_t> psf_fpixel(psf_shape.size(),0); // from (0,0)

	// define the first pixel that psf data will be written to
	std::vector<size_t> psf_fpixel_obs(obs_shape);
	LOGV_DEBUG(du::multiply(psf_fpixel_obs, 0.5));
	LOGV_DEBUG(du::multiply(psf_shape, 0.5));
	du::subtract_inplace(du::multiply_inplace(psf_fpixel_obs, 1./2), du::multiply(psf_shape, 1./2));
	LOGV_DEBUG(obs_shape);
	LOGV_DEBUG(psf_fpixel_obs);
	LOGV_DEBUG(psf_shape);
	LOGV_DEBUG(psf_fpixel);


	LOG_DEBUG("Copying PSF data to padded array");
	du::copy_to_rect(psf_data, padded_psf_data, psf_shape, obs_shape, psf_fpixel, psf_fpixel_obs);
	du::write_as_image(_sprintf("./plots/%psf_padded_before.pgm", tag), padded_psf_data, obs_shape);

	LOG_DEBUG("Removing NANs from padded_psf_data");
	// remove NANs from padded_psf_data
	std::vector<bool> psf_nan_mask = du::mask_where(padded_psf_data, std::function<bool(double)>(du::isnan<double>));
	du::set_at_mask(padded_psf_data, psf_nan_mask, 0.0);
	du::multiply_inplace(padded_psf_data, 1.0/du::sum(padded_psf_data));

	LOG_DEBUG("Adjusted padded_psf_data for convolution centering");
	// Re-center the padded_psf_data so that the convolution in "run()" 
	// is performed in the correct way.
	std::vector<double> delta_f(psf_fft.size(), 0);

	LOGV_DEBUG(padded_psf_data.size());
	LOGV_DEBUG(psf_fft.size(), delta_f.size(), du::idx_max(padded_psf_data));

	// Because of how fftw works, need to align on 0th pixel
	// we DO NOT want the PSF to be centered in it's frame
	//delta_f[delta_f.size() - du::idx_max(padded_psf_data)] = 1;
	//padded_psf_data = du::real_part(ifft(du::multiply(fft(padded_psf_data), fft(delta_f))));
	du::shift_inplace(padded_psf_data, padded_psf_data.size()/2);

	/* DEBUGGING write out PSF for testing
	std::vector<std::byte> temp = image_as_bytes(padded_psf_data);
	//std::vector<std::byte> temp = image_as_bytes(delta_f);
	LOGV_DEBUG(temp);
	//for(auto& item : temp){
	//	std::cout << item << " ";
	//}
	EM_ASM({(
		console.log("TESTING EM_ASM");
		console.log($0, $1, $2, $3);
		let im_data = new ImageData(new Uint8ClampedArray(Module.HEAPU8.buffer, $0, $1), $2, $3);
		scratch_canvas.width = $2;
		scratch_canvas.height = $3;
		scratch_canvas_ctx.imageSmoothingEnabled = false;
		console.log(`${scratch_canvas_ctx.imageSmoothingEnabled}`);
		scratch_canvas_ctx.putImageData(im_data, 0, 0);
	)}, temp.data(), temp.size(), obs_shape[0], obs_shape[1]);
	*/
	
}

void CleanModifiedAlgorithm::__str__(){
	GET_LOGGER;
	LOGV_DEBUG(n_iter);
	LOGV_DEBUG(n_positive_iter);
	LOGV_DEBUG(loop_gain);
	LOGV_DEBUG(threshold);
	LOGV_DEBUG(clean_beam_gaussian_sigma);
	LOGV_DEBUG(add_residual);
	LOGV_DEBUG(noise_std);
	LOGV_DEBUG(rms_frac_threshold);
	LOGV_DEBUG(fabs_frac_threshold);
	LOGV_DEBUG(residual_data);
	LOGV_DEBUG(components_data);
	LOGV_DEBUG(px_choice_map);
}

void CleanModifiedAlgorithm::_calc_pixel_threshold(){
	//px_threshold = threshold * du::max(residual_data);
	px_threshold = threshold * du::absmax(residual_data);
}

void CleanModifiedAlgorithm::_select_update_pixels(){
	px_choice_map = du::mask_where(residual_data,  std::function<bool(double)>([this](double v)->bool{return(abs(v) > px_threshold);}));
	du::set_to(selected_pixels, 0.0);
	du::set_at_mask(selected_pixels, px_choice_map, residual_data);
}

std::pair<
	std::vector<double>,
	std::vector<size_t>
> CleanModifiedAlgorithm::_ensure_odd(
	const std::vector<double>& obs_data, 
	const std::vector<size_t>& obs_shape
){
	std::vector<size_t> alter_obs_shape(obs_shape.size(),0);
	for(size_t i=0; i<obs_shape.size(); ++i){
		alter_obs_shape[i] = 1 - obs_shape[i]%2;
	}
	std::vector<size_t> new_obs_shape(obs_shape);
	du::add_inplace(new_obs_shape, alter_obs_shape);

	std::vector<double> new_obs_data(du::product(new_obs_shape), 0);
	
	std::vector<size_t> zero(obs_shape.size(),0);

	du::copy_to_rect(obs_data, new_obs_data, obs_shape, new_obs_shape, zero, zero);

	return std::make_pair(new_obs_data, new_obs_shape);
}

void CleanModifiedAlgorithm::run(
		std::vector<double>& obs_data, 
		std::vector<size_t> obs_shape, 
		const std::vector<double>& psf_data, 
		const std::vector<size_t>& psf_shape,
		const std::string& run_tag
	){
	GET_LOGGER;
	LOG_DEBUG("declare variables");

	
	tag=run_tag;

	size_t n_selected_pixels = 0;

	std::pair<std::vector<double>, std::vector<size_t>> ret = _ensure_odd(obs_data, obs_shape);
	obs_data = ret.first;
	obs_shape = ret.second;


	LOG_DEBUG("resize dynamic arrays");
	// resize arrays to hold desired data
	padded_psf_data.resize(obs_data.size());
	psf_fft.resize(obs_data.size());
	selected_pixels.resize(obs_data.size());
	px_choice_map.resize(obs_data.size());
	selected_px_fft.resize(obs_data.size());
	current_convolved.resize(obs_data.size());
	components_data.resize(obs_data.size());

	du::multiply_inplace(components_data, 0);

	LOG_DEBUG("set FFT attributes");
	// set attributes for fourier transformers
	//fft.set_attrs(obs_shape, false, true);
	fft.set_attrs(obs_shape, false, false);
	LOG_DEBUG("forward fft attributes set");
	//ifft.set_attrs(obs_shape, true, true);
	ifft.set_attrs(obs_shape, true, false);
	LOG_DEBUG("backward fft attributes set");

	LOG_DEBUG("Getting residual from obs_data");
	_get_residual_from_obs(obs_data, obs_shape);

	LOG_DEBUG("Padding PSF data");
	_get_padded_psf(psf_data, obs_shape, psf_shape);
		
	LOG_DEBUG("precompute PSF FFT");
	// get the FFT of the PSF, will need it later
	psf_fft = fft(padded_psf_data);

	du::write_as_image(_sprintf("./plots/%psf_raw.pgm", tag), psf_data, psf_shape);
	du::write_as_image(_sprintf("./plots/%obs_raw.pgm", tag), obs_data, obs_shape);

	du::write_as_image(_sprintf("./plots/%psf_padded.pgm", tag), padded_psf_data, obs_shape);

	du::write_as_image(_sprintf("./plots/%psf_fft_real.pgm", tag), du::real_part(psf_fft), obs_shape);
	du::write_as_image(_sprintf("./plots/%psf_fft_imag.pgm", tag), du::imag_part(psf_fft), obs_shape);

	// print the backward fft of the psf_fft to see if it agrees with the original data.
	auto psf_fft_ifft = ifft(psf_fft);
	du::write_as_image(_sprintf("./plots/%psf_fft_ifft_real.pgm", tag), du::real_part(psf_fft_ifft), obs_shape);
	du::write_as_image(_sprintf("./plots/%psf_fft_ifft_imag.pgm", tag), du::imag_part(psf_fft_ifft), obs_shape);


	
	LOG_DEBUG("Starting deconvolution n_iter %", n_iter);

	timer::start();
	for(size_t i=0; i<n_iter; ++i){
		LOGV_DEBUG(i);
		
		emscripten_sleep(1);

		_calc_pixel_threshold();
		_select_update_pixels();
		
		n_selected_pixels = 0;
		for(auto item : selected_pixels){
			n_selected_pixels += item ? 1 : 0;
		}
		du::multiply_inplace(selected_pixels, loop_gain);

		selected_px_fft = fft(selected_pixels);

		current_convolved = du::real_part(ifft(du::multiply(selected_px_fft, psf_fft)));

		if (!(i%1)){
			LOGV_DEBUG(i);
			du::write_as_image(_sprintf("./plots/%components_%.pgm", tag, i), components_data, obs_shape);
			du::write_as_image(_sprintf("./plots/%residual_%.pgm", tag, i), residual_data, obs_shape);
			du::write_as_image(_sprintf("./plots/%selected_pixels_%.pgm", tag, i), selected_pixels, obs_shape); 
			//du::write_as_image(_sprintf("./plots/%selected_px_fft_real_%.pgm", tag, i), du::real_part(selected_px_fft), obs_shape); 
			//du::write_as_image(_sprintf("./plots/%selected_px_fft_imag_%.pgm", tag, i), du::imag_part(selected_px_fft), obs_shape); 
			//du::write_as_image(_sprintf("./plots/%selected_pixels_fft_ifft_%.pgm", tag, i), du::real_part(ifft_obs_shape(selected_px_fft)), obs_shape); 
			du::write_as_image(_sprintf("./plots/%current_convolved_%.pgm", tag, i), current_convolved, obs_shape); 
			
			const std::span<std::byte>& temp = image_as_blob(BLOB_ID_COMPONENTS, components_data);
			send_to_js_canvas(temp.data(), temp.size(), obs_shape[0], obs_shape[1]);
		}

		//LOGV_DEBUG(du::sum(current_convolved));
		//LOGV_DEBUG(du::sum(components_data));

		du::subtract_inplace(residual_data, current_convolved);
		//du::add_inplace(components_data, current_convolved);
		du::add_inplace(components_data, selected_pixels);

		fabs_record[i] = du::max(du::apply(residual_data, abs ));
		rms_record[i] = sqrt(du::sum(du::apply(residual_data, du::square ))/residual_data.size());
		//LOGV_DEBUG(fabs_record[i]);
		//LOGV_DEBUG(rms_record[i]);
	}

	du::write_as_image(_sprintf("./plots/%components.pgm", tag), components_data, obs_shape);
	du::write_as_image(_sprintf("./plots/%residual.pgm", tag), residual_data, obs_shape);
	du::write_as_image(_sprintf("./plots/%residual_log.pgm", tag), du::log(residual_data), obs_shape);

	if (clean_beam_gaussian_sigma > 0){
		LOG_DEBUG("Convolving result with gaussian clean beam with sigma=%", clean_beam_gaussian_sigma);
		Eigen::MatrixXd Kernel(obs_shape[0], obs_shape[1]);
		std::vector<FourierTransformer::complex> Kernel_fft(Kernel.size());
		std::vector<FourierTransformer::complex> components_data_fft(Kernel.size());

		Eigen::Matrix<double, 2,2> Sigma {	{1.0/(clean_beam_gaussian_sigma*clean_beam_gaussian_sigma), 0},
											{0, 1.0/(clean_beam_gaussian_sigma*clean_beam_gaussian_sigma)}
											};
		
		// As fftw produces non-centered FFTs, pos should be difference from center,
		// however, will that account for everything correctly?
		// I don't think so, I should really adjust the Kernel after it's created
		// so that it is re-centered on (0,0) instead of (obs_shape[0]/2, obs_shape[1]/2).
		//Eigen::Vector2d pos{0,0};
		Eigen::Vector2d pos{obs_shape[0]/2.0,obs_shape[1]/2.0}; 
		
		Eigen::Vector2d idx {0,0};

		for(int i=0;i < Kernel.rows(); idx[0]+=1, ++i){
			for(int j=0; j<Kernel.cols(); idx[1]+=1, ++j){
				Kernel(i,j) = exp(-static_cast<double>((idx-pos).transpose() * Sigma * (idx-pos)));
				//LOG_DEBUG("Kernel(%,%) = %",i,j,Kernel(i,j));
				//LOGV_DEBUG(idx);
				//LOGV_DEBUG(Sigma);
				//LOGV_DEBUG(-static_cast<double>((idx-pos).transpose() * Sigma * (idx-pos)));
			}
			idx[1] = 0;
		}
		
		Kernel.array() /= sqrt(M_PI*M_PI/Sigma.determinant());
		LOGV_DEBUG(Kernel.sum());
		//LOGV_DEBUG(pos);
	
		du::write_as_image(_sprintf("./plots/%Kernel.pgm", tag), std::vector<double>(Kernel.data(), Kernel.data()+Kernel.size()), obs_shape);

		// TODO:
		// * Re-center Kernel on (0,0) so that the fft-convolution doesn't go weird.
		std::vector<double> temp(Kernel.size(), 0);
		temp[temp.size() - (pos[0]*Kernel.colStride() + pos[1]*Kernel.rowStride())] = 1;


		Kernel_fft = fft(std::vector<double>(Kernel.data(), Kernel.data()+Kernel.size()));
		du::write_as_image(_sprintf("./plots/%Kernel_fft_real.pgm", tag), du::real_part(Kernel_fft), obs_shape);
		du::write_as_image(_sprintf("./plots/%Kernel_fft_imag.pgm", tag), du::imag_part(Kernel_fft), obs_shape);
			
		du::write_as_image(_sprintf("./plots/%Kernel_fft_ifft_real.pgm", tag), du::real_part(ifft(Kernel_fft)), obs_shape);
		du::write_as_image(_sprintf("./plots/%Kernel_fft_ifft_imag.pgm", tag), du::imag_part(ifft(Kernel_fft)), obs_shape);
		
		components_data_fft = fft(components_data);
		clean_map = du::real_part(ifft(du::multiply(components_data_fft, du::multiply(Kernel_fft,fft(temp)))));
		LOGV_DEBUG(du::sum(components_data));
		LOGV_DEBUG(du::max(components_data));
		LOGV_DEBUG(du::sum(clean_map));
		LOGV_DEBUG(du::max(clean_map));
	}
	else {
		LOG_DEBUG("Convolution with clean-beam not performed");
		clean_map = components_data;
	}


	if(add_residual){
		LOG_DEBUG("Adding residual to clean map");
		du::add_inplace(clean_map, residual_data);
	} else {
		LOG_DEBUG("Residual NOT added to clean map");
	}


	du::write_as_image(_sprintf("./plots/%clean_map.pgm",tag), clean_map, obs_shape);

	timer::stop();
	LOGV_DEBUG(timer::n_seconds());
}





