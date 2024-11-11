#ifndef __DECONV_INCLUDED__
#define __DECONV_INCLUDED__

//#include <iostream>
#include "Eigen/Dense"
#include "Eigen/LU"
//#include <cassert>
//#include <string>
//#include <vector>
//#include <map>
//#include "fitsio.h"
//#include <fftw3.h>
//#include <complex>
//#include <cmath>
//#include <functional>

#include "emscripten.h"

//#include "logging.h"
//#include "format.hpp"
#include "data_utils.hpp"

//#include <span>

//#include "emscripten/bind.h"
#include "emscripten/val.h"
#include "fft.hpp"
#include "storage.hpp"
#include "js_glue.hpp"
//#include "tiff_helper.hpp"

namespace du = data_utils;

extern "C" void send_to_js_canvas(void* ptr, int size, int width, int height);


struct ParameterInformation{
	std::string name;
	std::string description;
	std::string type;
	std::string domain;
	std::string default_value;
};


class CleanModifiedAlgorithm {
	public:

	
	Storage::blob_id_t BLOB_ID_COMPONENTS=Storage::BlobMgr::get_free_id();
	Storage::blob_id_t BLOB_ID_CLEAN_MAP=Storage::BlobMgr::get_free_id();
	Storage::blob_id_t BLOB_ID_RESIDUAL=Storage::BlobMgr::get_free_id();

	size_t n_iter;
	size_t n_positive_iter;
	double loop_gain;
	double threshold;
	double clean_beam_gaussian_sigma;
	bool add_residual;
	double noise_std;
	double rms_frac_threshold;
	double fabs_frac_threshold;
	size_t data_size;
	std::vector<size_t> data_shape;
	std::vector<double> residual_data;
	std::vector<double> components_data;
	std::vector<double> clean_map;
	std::vector<bool> px_choice_map;

	double px_threshold;
	std::vector<double> padded_psf_data;
	std::vector<double> selected_pixels;
	std::vector<double> current_convolved;
	std::vector<double> fabs_record;
	std::vector<double> rms_record;
	FourierTransformer fft;
	FourierTransformer ifft;

	std::vector<FourierTransformer::complex> psf_fft;
	std::vector<FourierTransformer::complex> selected_px_fft;

	std::string tag;

	CleanModifiedAlgorithm(
		size_t _n_iter = 1000,
		size_t _n_positive_iter = 0,
		double _loop_gain = 0.1,
		double _threshold = 0.3,
		//double _clean_beam_gaussian_sigma = 2.0,
		double _clean_beam_gaussian_sigma = 0.0,
		//bool _add_residual = true,
		bool _add_residual = false,
		double _noise_std = 1E-2,
		double _rms_frac_threshold = 1E-2,
		double _fabs_frac_threshold = 1E-2
	);

	void _get_residual_from_obs(const std::vector<double>& obs_data, const std::vector<size_t>& obs_shape);
	void _get_padded_psf(const std::vector<double>& psf_data, const std::vector<size_t>& obs_shape, const std::vector<size_t>& psf_shape);
	void __str__();
	void _calc_pixel_threshold();
	void _select_update_pixels();

	std::pair<
		std::vector<double>,
		std::vector<size_t>
	> _ensure_odd(const std::vector<double>& obs_data, const std::vector<size_t>& obs_shape);

	void doIter(
		size_t i
	);
	
	void prepare_observations(
		std::vector<double>& obs_data, 
		std::vector<size_t> obs_shape, 
		const std::vector<double>& psf_data, 
		const std::vector<size_t>& psf_shape,
		const std::string& run_tag=""
	);
	
	void run();

};




#endif //__DECONV_INCLUDED__
