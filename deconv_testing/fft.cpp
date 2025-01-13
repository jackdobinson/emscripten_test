#include "fft.hpp"


// TODO: 
// * Document quirks, e.g., centering is around 1st pixel when convolving
FourierTransformer::FourierTransformer(
		const std::vector<size_t>& _shape,
		const bool _inverse,
		const bool _plan_measure
	) : shape(du::reverse(_shape)), size(du::product(shape)), inverse(_inverse), plan_measure(_plan_measure)
{
	get_plan();
}

void FourierTransformer::set_attrs(
	const std::vector<size_t>& _shape,
	const bool _inverse,
	const bool _plan_measure
){
	GET_LOGGER;
	shape = du::reverse(_shape);

	size = du::product(shape);
	inverse = _inverse;
	plan_measure = _plan_measure;
	LOGV_DEBUG(shape, size, inverse, plan_measure);

	get_plan();
}

void FourierTransformer::get_plan(){
	GET_LOGGER;

	in.resize(du::product(shape));
	out.resize(in.size());
	LOG_DEBUG("Getting Plan");
	plan = fftw_plan_dft(
		shape.size(),
		du::as_type<int>(shape).data(),
		reinterpret_cast<fftw_complex*>(in.data()),
		reinterpret_cast<fftw_complex*>(out.data()),
		(inverse) ? FFTW_BACKWARD : FFTW_FORWARD,
		(plan_measure) ? FFTW_MEASURE : FFTW_ESTIMATE
	);
}




