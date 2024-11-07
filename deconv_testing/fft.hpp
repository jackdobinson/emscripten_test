#ifndef __FFT_INCLUDED__
#define __FFT_INCLUDED__

#include <vector>
#include <fftw3.h>
#include <complex>
#include <cmath>
#include "data_utils.hpp"
#include "logging.h"

namespace du = data_utils;

class FourierTransformer{
	public:
	using complex=std::complex<double>;
	
	std::vector<complex> in, out;
	std::vector<size_t> shape;
	size_t size;
	bool inverse;
	bool plan_measure;
	fftw_plan plan;

	FourierTransformer(
			const std::vector<size_t>& _shape = {},
			const bool _inverse = false,
			const bool _plan_measure = false
		);
	
	void set_attrs(
		const std::vector<size_t>& _shape,
		const bool _inverse = false,
		const bool _plan_measure = false
	);
	
	void get_plan();


	template <class T>
	std::vector<complex>& operator()(const std::vector<T>& input_data){
		assert(input_data.size() == size);

		if constexpr(std::is_same<complex, T>::value) {
			in = input_data;
		}
		else if constexpr(du::is_template_specialisation<T, std::complex>{}){
			du::copy_to(input_data, in);
		} else {
			// Assume input data is real
			du::copy_as_real(input_data, in);
		}

		fftw_execute(plan);

		if (inverse){
			du::multiply_inplace(out, 1.0/size);
		}

		return(out);
	}
};

#endif //__FFT_INCLUDED__
