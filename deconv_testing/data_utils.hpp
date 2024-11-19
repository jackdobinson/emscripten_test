#ifndef __DATA_UTILS_INCLUDED__
#define __DATA_UTILS_INCLUDED__

#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <functional>
#include <cmath>
#include <bit>
#include <complex>
#include <ios>
#include <iomanip>
#include <fstream>
#include <chrono>

#include "logging.h"
#include "str_printf.h"

/*
extern "C" {
	// Link with "-lnetpbm"
	#include <pam.h>
	// Undefine macros to avoid name collisions
	#undef max
	#undef min
}
*/
#define BITSHIFT_MASK(N) 0xFF << ((N)*8)

// Define size_t incase header files don't define it. This is identical to the "real" definition
// and if it's not it should throw a compiler error.
using size_t = decltype(sizeof(int));

namespace timer {
	void start();
	void stop();
	double n_seconds();
}


namespace template_utils{
	template<class... Ts> 
	auto product_pp(Ts ... args){
		return(args * ... * 1);
	}
}

namespace data_utils{

	// MEMBERSHIP TESTS
	
	template<class T>
	bool contains(const std::vector<T>& a, const T& b){
		for (const T& item : a){
			if (item == b){
				return true;
			}
		}
		return false;
	}

	// MULTIPLY ARRAYS

	template<class T1, class T2>
	std::vector<T1>  multiply(const std::vector<T1>& a, const std::vector<T2> b){
		assert(a.size() == b.size());
		std::vector<T1> r(a.size());

		for(size_t i=0; i<a.size(); ++i){
			r[i] = a[i] * b[i];
		}
		return(r);
	}

	template <class T1, class T2>
	std::vector<T1> multiply(const T1* aPtr, const size_t an, const T2* bPtr, const size_t bn){
		assert(an==bn);
		std::vector<T1> r(an);
		T1 *rPtr = r.data(), *rPtrEnd=r.data()+r.size();
		for(;rPtr < rPtrEnd; ++aPtr, ++bPtr, ++rPtr){
			(*rPtr) = (*aPtr) * (*bPtr);
		}
		return(r);
	}

	template<class T1, class T2>
	std::vector<T1>  multiply(const std::vector<T1>& a, const T2 v){
		std::vector<T1> r(a.size());

		for(size_t i=0; i<a.size(); ++i){
			r[i] = a[i] * v;
		}
		return(r);
	}
	template<class T1, class T2>
	std::vector<T1>& multiply_inplace(std::vector<T1>& a, const std::vector<T2> b){
		assert(a.size() == b.size());

		for(size_t i=0; i<a.size(); ++i){
			a[i] *= b[i];
		}
		return(a);
	}
	template<class T1, class T2>
	std::vector<T1>& multiply_inplace(std::vector<T1>& a, const T2 v){
		for(size_t i=0; i<a.size(); ++i){
			a[i] *= v;
		}
		return(a);
	}

	// APPLY FUNCTION TO ARRAYS

	template<typename T>
	using KernelFunction = T(*)(T);

	template<class T>
	std::vector<T> apply(const std::vector<T>& a, KernelFunction<T> func){
		std::vector<T> r(a.size());
		for(size_t i=0;i<a.size(); ++i){
			r[i] = func(a[i]);
		}
		return(r);
	}
	template<class T>
	void apply_inplace(std::vector<T>& a, KernelFunction<T> func){
		for(size_t i=0;i<a.size(); ++i){
			a[i] = func(a[i]);
		}
	}

	// ARRAY MASKING

	// Create Array Mask
	template <class T>
	std::vector<bool> mask_where(const std::vector<T>& a, std::function<bool(T)>&& condition_func){
		std::vector<bool> mask(a.size());
		for (size_t i=0; i<a.size(); ++i){
			mask[i] = condition_func(a[i]);
		}
		return(mask);
	}

	// Set Arrays At Mask
	template<class T, class M>
	void set_at_mask(std::vector<T>& a, const std::vector<M> mask, const T value){
		assert(a.size() == mask.size());
		for (size_t i = 0; i<a.size(); ++i){
			if (mask[i]){
				a[i] = value;
			}
		}
	}
	template<class T, class M>
	void set_at_mask(std::vector<T>& a, const std::vector<M> mask, const std::vector<T>& b){
		assert(a.size() == mask.size());
		for (size_t i = 0; i<a.size(); ++i){
			if (mask[i]){
				a[i] = b[i];
			}
		}
	}

	// Invert Arry Mask
	template<class M>
	void mask_invert(std::vector<M> mask){
		for (auto item : mask){
			item = !item;
		}
	}


	// DEALING WITH ENDIANESS
	
	// Union for endian testing
	static const union {
		uint16_t value;
		uint8_t bytes[2];
	} ENDIAN_TEST = {255};

	// flag for testing bigendianness at runtime
	static const bool is_bigendian = ENDIAN_TEST.bytes[0] > 0;

	// Byteswapping between endian types
	template<class T, uint8_t N>
	inline T __byteswap_helper(T value){
		if constexpr(N == 1){
			return ((value & BITSHIFT_MASK(N-1))  << ((sizeof(T) - 1)*8));
		}
		else if constexpr(N <= sizeof(T)/2){
			return ((value & BITSHIFT_MASK(N-1))  << ((-2*N + sizeof(T) + 1)*8) | __byteswap_helper<T,N-1>(value));
		}
		else{
			return ((value & BITSHIFT_MASK(N-1))  >> ((2*N - sizeof(T) - 1)*8) | __byteswap_helper<T,N-1>(value));
		}
	}

	template<class T>
	T byteswap(T value){
		return(__byteswap_helper<T, sizeof(T)>(value));
	}


	// CHECK FOR TEMPLATE TYPES
	template <class, template <class...> class>
	struct is_template_specialisation_impl : public std::false_type {};

	template <template <class...> class U, class ... Ts>
	struct is_template_specialisation_impl<U<Ts...>, U> : public std::true_type {};

	template <class T, template <class ...> class U>
	using is_template_specialisation = is_template_specialisation_impl<std::remove_cv_t<T>,U>;

	// ARRAY ACCUMULATION OPERATIONS

	template<class T>
	T product(std::vector<T> a){
		// Multipy all values in array together
		T accumulator(1);
		for (auto item : a){
			accumulator *= item;
		}
		return(accumulator);
	}

	template<class T>
	size_t idx_max(const std::vector<T>& a){
		assert(a.size() >0);
		size_t idx=0;
		T max(a[0]);
		for(size_t i=0; i< a.size(); ++i){
			if (a[i] > max){
				max = a[i];
				idx = i;
			}
		}
		return(idx);
	}

	template<class T>
	T max(const std::vector<T>& a){
		// Find max element of array using ">" operator
		assert(a.size() > 0);
		T max_value(a[0]);
		for (auto elem : a){
			if (elem > max_value){
				max_value = elem;
			}
		}
		return(max_value);
	}

	template<class T>
	T max(T* ptr, size_t n){
		// Find max value of a c-style array using ">" operator
		assert(n > 0);
		T max_value(*ptr);
		T* endPtr(ptr+n);
		for(; ptr<endPtr; ++ptr){
			if((*ptr) > max_value){
				max_value = *ptr;
			}
		}
		return(max_value);
	}

	template<class T>
	T absmax(const std::vector<T>& a){
		// Find absolute max element of array using ">" operator
		assert(a.size() > 0);
		T max_value(a[0]);
		for (auto elem : a){
			if (abs(elem) > abs(max_value)){
				max_value = elem;
			}
		}
		return(max_value);
	}

	template<class T>
	T absmax(T* ptr, size_t n){
		// Find absolute max value of a c-style array using ">" operator
		assert(n > 0);
		T max_value(*ptr);
		T* endPtr(ptr+n);
		for(; ptr<endPtr; ++ptr){
			if(abs((*ptr)) > abs(max_value)){
				max_value = *ptr;
			}
		}
		return(max_value);
	}
	template<class T>
	T min(const std::vector<T>& a){
		// Find min element of array using "<" operator
		assert(a.size() > 0);
		T min_value(a[0]);
		for (auto elem : a){
			if(elem < min_value){
				min_value = elem;
			}
		}
		return(min_value);
	}

	template<class T>
	T sum(const std::vector<T>& a){
		// Add up all elements in array
		T sum=0;
		for(auto item : a){
			sum += item;
		}
		return(sum);
	}

	// SET ARRAY ELEMENTS

	template<class T>
	void set_to(std::vector<T>& a, T value){
		for (auto elem : a){
			elem = value;
		}
	}

	template<class T>
	void set_linspace(std::vector<T>& a, T min_value, T max_value, size_t n_values){
		T step((max_value - min_value)/n_values);
		for (size_t i=0; i<a.size(); ++i){
			a[i] = min_value + i*step;
		}
	}

	template<class T>
	void set_logspace(std::vector<T>& a, T min_value, T max_value, size_t n_values){
		T factor(max_value/min_value);
		T factor_step((factor - 1)/n_values);
		for (size_t i=0; i<a.size(); ++i){
			a[i] = min_value * (1+factor_step);
		}
	}

	// CUMULANTS
	template <class T>
	std::vector<T> cumulative_product(const std::vector<T>& a){
		std::vector<T> r(a);
		for(size_t i=1; i<r.size(); ++i){
			r[i] *= r[i-1];
		}
		return(r);
	}
	template <class T>
	std::vector<T>& cumulative_product_inplace(std::vector<T>& a){
		for(size_t i=1; i<a.size(); ++i){
			a[i] *= a[i-1];
		}
		return(a);
	}
	template <class T>
	std::vector<T> cumulative_sum(const std::vector<T>& a){
		std::vector<T> r(a);
		for(size_t i=1; i<r.size(); ++i){
			r[i] += r[i-1];
		}
		return(r);
	}
	
	// REVERSING
	

	template <class T>
	std::vector<T>& reverse_inplace(std::vector<T>& a){
		GET_LOGGER;
		LOGV_DEBUG(a);
		T temp;
		size_t N = a.size();
		for(size_t i=0; i<N/2; ++i){
			LOGV_DEBUG(i, N, N/2);
			temp = a[i];
			a[i] = a[N-1-i];
			a[N-1-i] = temp;
		}
		return(a);
	}
	
	template <class T>
	std::vector<T> reverse(const std::vector<T>& a){
		std::vector<T> r(a);
		return(reverse_inplace(r));
	
	}

	template <class T>
	std::vector<T>& shift_inplace(std::vector<T>& a, size_t n){
		// shift vector along e.g. {1,2,3,4} -> {2,3,4,1}
		std::vector<T> temp(n);
		size_t i;

		if (n > a.size()){
			n = n % a.size();
		}

		for (i=0; i<n; i++){
			temp[i] = a[i];
		}
		for(i=0; i<a.size()-n; i++){
			a[i] = a[i+n];
		}
		for(i=0; i<n; i++){
			a[i + a.size() - n] = temp[i];
		}

		return a;
	}

	// SHAPES
	
	
	
	template <class T>
	std::vector<T> get_strides(const std::vector<T>& shape){
		// Assume fastest varying axis is on LHS
		std::vector<T> strides(shape); // {430, 427, 461}
		shift_inplace(strides, strides.size()-1); //{461, 430, 427}
		strides[0] = 1;
		cumulative_product_inplace(strides); // {1, 430, 427*430}
		return(strides);	
	}

	template<class T1, class T2>
	size_t index_nd_to_1d(const std::vector<T1>& shape, const std::vector<T2>& idxs){
		return(sum(multiply(get_strides(shape), idxs)));
	}

	template <class T1, class T2>
	std::vector<size_t> index_1d_to_nd(const std::vector<T1>& shape, T2 idx){
		std::vector<T1> strds = get_strides(shape);
		std::vector<T2> idxs(shape.size());
		for(size_t i=strds.size()-1; i>0; --i){
			idxs[i] = idx/strds[i];
			idx -= idxs[i]*strds[i];
		}
		idxs[0] = idx;
		return(idxs);
	}

	template<class T1, class T2, class T3>
	size_t index_nd_to_1d_strides(const std::vector<T1>& strides, const std::vector<T2>& idxs){
		return(sum(multiply(strides, idxs)));
	}

	template <class T1, class T2, class T3>
	std::vector<size_t> index_1d_to_nd(const std::vector<T1>& shape, T2 idx, const std::vector<T3>& strides){
		std::vector<T2> idxs(shape.size());
		for(size_t i=strides.size()-1; i>0; --i){
			idxs[i] = idx/strides[i];
			idx -= idxs[i]*strides[i];
		}
		idxs[0] = idx;
		return(idxs);
	}

	// COPYING ARRAYS

	template<class T>
	void copy_to(const std::vector<T>& a, std::vector<T>& b, size_t from_idx=0, size_t to_idx=0){
		size_t N = ((a.size()-from_idx) > (b.size()-to_idx))? (b.size()-to_idx) : (a.size()-from_idx);
		for(size_t i=0; i<N; ++i){
			b[i+to_idx] = a[i+from_idx];
		}
	}

	template<class T>
	void copy_to_rect(
			const std::vector<T>& a, 
			std::vector<T>& b, 
			const std::vector<size_t>& a_shape, 
			const std::vector<size_t>& b_shape, 
			const std::vector<size_t>& a_fpixel, 
			const std::vector<size_t>& a_fpixel_b,
			const int mode=0 //0 - clip, 1 - wrap around
		){
		// a - smaller array to copy from
		// b - larger array to copy to
		// a_fpixel - starting point in a to copy from
		// a_fpixel_b - starting point in b to copy to
		GET_LOGGER;
		size_t a_n_px = product(a_shape);
		size_t b_n_px = product(b_shape);
		assert(a_n_px <= b_n_px);
		assert((a_shape.size() == b_shape.size()) && (a_shape.size() < (short)(-1)));

		size_t j_f = index_nd_to_1d(a_shape, a_fpixel); // index of first pixel in a
		size_t i_f = index_nd_to_1d(b_shape, a_fpixel_b); // index of first pixel in b
		
		std::vector<size_t> a_strides = get_strides(a_shape);
		std::vector<size_t> b_strides = get_strides(b_shape);

		std::vector<size_t> a_idxs = a_fpixel;
		std::vector<size_t> b_idxs = a_fpixel_b;

		//LOGV_DEBUG(a_fpixel, a_fpixel_b);
		//LOGV_DEBUG(a_n_px, b_n_px);
		//LOGV_DEBUG(j_f, i_f);
		//LOGV_DEBUG(a_strides);
		//LOGV_DEBUG(b_strides);
		//LOGV_DEBUG(b_idxs);
		//LOGV_DEBUG(a_shape);
		//LOGV_DEBUG(b_shape);

		bool idx_in_b = true;
		size_t i = i_f;
		size_t l = 0;
		short axis_iterated = 0;
		//LOG_DEBUG("Starting loop");
		for (size_t j=j_f; j<a_n_px; ++j){
			//LOGV_DEBUG(j);
			a_idxs = index_1d_to_nd(a_shape, j);
			//LOGV_DEBUG(a_idxs);
			
			idx_in_b = true;
			for(short k=0; k<b_shape.size(); ++k){
				b_idxs[k] = a_fpixel_b[k] + (a_idxs[k] - a_fpixel[k]);
				if(b_idxs[k] >= b_shape[k]) {
					switch(mode){
						case 0:{
							// Clip copy from a to b
							idx_in_b = false;
							break;
						}
						case 1:{
							// wrap copy from a to b
							b_idxs[k] = b_idxs[k] % b_shape[k];
							break;
						}
						default:{
							LOG_ERROR("Unknown mode '%'. Should be one of {0 (clip), 1 (wrap around)}", mode);
							break;
						}
					}
				}
			}
			//LOGV_DEBUG(b_idxs);

			if (idx_in_b){
				i = index_nd_to_1d(b_shape, b_idxs);	
				b[i] = a[j];
			}
			//LOGV_DEBUG(i);

		}
	}

	template<class T>
	void copy_as_real(const std::vector<T>& source, std::vector<std::complex<T>>& complex_dest){
		assert(source.size() == complex_dest.size());
		for(size_t i=0; i<source.size(); ++i){
			complex_dest[i].real(source[i]);
		}
		return;
	}

	template<class T>
	void copy_as_imag(const std::vector<T>& source, std::vector<std::complex<T>>& complex_dest){
		assert(source.size() == complex_dest.size());
		for(size_t i=0; i<source.size(); ++i){
			complex_dest[i].imag(source[i]);
		}
		return;
	}

	template<class T>
	void copy_from_real(const std::vector<std::complex<T>>& source, std::vector<T>& real_dest){
		assert(source.size() == real_dest.size());
		for(size_t i=0; i<source.size(); ++i){
			real_dest[i] = source[i].real();
		}
		return;
	}

	template<class T>
	void copy_from_imag(const std::vector<std::complex<T>>& source, std::vector<T>& imag_dest){
		assert(source.size() == imag_dest.size());
		for(size_t i=0; i<source.size(); ++i){
			imag_dest[i] = source[i].imag();
		}
		return;
	}

	// RESHAPE ARRAYS
	
	template <class T1, class T2, class T3>
	std::vector<T1> reshape(
			const std::vector<T1>& data, 
			const std::vector<T2>& original_shape, 
			const std::vector<T3>& new_shape
		){
		std::vector<size_t> fpixel_data(original_shape.size(), 0);
		std::vector<size_t> fpixel_reshaped_data(new_shape.size(), 0);
		size_t new_size = product(new_shape);
		std::vector<T1> reshaped_data(new_size, 0);
		
		
		copy_to_rect(data, reshaped_data, original_shape, new_shape, fpixel_data, fpixel_reshaped_data);
		return reshaped_data;
	}

	// SUBTRACT ARRAYS

	template<class T1, class T2>
	std::vector<T1>& subtract_inplace(std::vector<T1>& a, const std::vector<T2>& b){
		for(size_t i=0; i<a.size(); ++i){
			a[i] -= b[i];
		}
		return(a);
	}
	template<class T1, class T2>
	std::vector<T1>& subtract_inplace(std::vector<T1>& a, const T2 v){
		for(size_t i=0; i<a.size(); ++i){
			a[i] -= v;
		}
		return(a);
	}

	template<class T1, class T2>
	std::vector<T1> subtract(const std::vector<T1>& a, const std::vector<T2>& b){
		std::vector<T1> r(a.size());
		copy_to(a,r);
		return(subtract_inplace(r, b));
	}

	template<class T1, class T2>
	std::vector<T1> subtract(const std::vector<T1>& a, const T2 v){
		std::vector<T1> r(a.size());
		copy_to(a,r);
		return(subtract_inplace(r, v));
	}

	// ADD ARRAYS

	template<class T1, class T2>
	std::vector<T1>& add_inplace(std::vector<T1>& a, const std::vector<T2>& b){
		for(size_t i=0; i<a.size(); ++i){
			a[i] += b[i];
		}
		return(a);
	}
	template<class T1, class T2>
	std::vector<T1>& add_inplace(std::vector<T1>& a, const T2 v){
		for(size_t i=0; i<a.size(); ++i){
			a[i] += v;
		}
		return(a);
	}

	template<class T1, class T2>
	std::vector<T1> add(const std::vector<T1>& a, const std::vector<T2>& b){
		std::vector<T1> r(a.size());
		copy_to(a,r);
		return(add_inplace(r, b));
	}

	template<class T1, class T2>
	std::vector<T1> add(const std::vector<T1>& a, const T2 v){
		std::vector<T1> r(a.size());
		copy_to(a,r);
		return(add_inplace(r, v));
	}



	// TYPE MANIPULATION
	template <class R, class T>
	std::vector<R> as_type(const std::vector<T>& a){
		std::vector<R> r(a.size());
		for(size_t i=0; i<r.size(); ++i){
			r[i] = a[i];
		}
		return(r);
	}

	// ACCESS PARTS OF COMPLEX ARRAYS

	template<class T>
	std::vector<T> real_part(const std::vector<std::complex<T>>& source){
		std::vector<T> r(source.size());
		copy_from_real(source, r);
		return(r);
	}

	template <class T>
	std::vector<T> imag_part(const std::vector<std::complex<T>>& source){
		std::vector<T> r(source.size());
		copy_from_imag(source, r);
		return(r);
	}



	// ARRAY UTILITY OPERATIONS
	template<class R, class T>
	R abs_greater_than(T value, T test){
		return(abs(value) > test);
	}


	template<class T>
	T square(T value){
		return(value*value);
	}

	template<class T>
	bool isnan(T value){
		return(std::isnan(value));
	}


	template <class T>
	std::vector<T>& nan_to_num_inplace(std::vector<T>& a){
		T *ptr(a.data()), *endPtr(a.data()+a.size());
		for(; ptr < endPtr; ++ptr){
			if(isnan(*ptr)){
				*ptr = 0;
			}
		}
		return(a);
	}

	template <class T>
	std::vector<T>& log_inplace(std::vector<T>& a){
		T *ptr(a.data()), *endPtr(a.data()+a.size());
		for(; ptr < endPtr; ++ptr){
			*ptr = log(*ptr);
		}
		return(a);
	}

	template <class T>
	std::vector<T> log(const std::vector<T>& a){
		std::vector<T> b(a.size());
		copy_to(a,b);
		log_inplace(b);
		return(b);
	}

	

	template<size_t N>
	struct StringLiteral {
		char value[N];

		constexpr StringLiteral(const char (&str)[N]){
			for(size_t i=0; i<N; ++i){
				value[i] = str[i];
			}
		}
	};

	// Output to file as image
	template <class T, class I=size_t, class VALUE_TYPE=uint16_t, StringLiteral TUPLE_TYPE="GRAYSCALE" >
	void write_as_image(const std::string& fpath, std::vector<T> a, const std::vector<I>& shape){
		// DEBUGGING
		printf("Would write image %s here\n", fpath.c_str());
		return
		/*
		 * Writes data in a vector to a PAM image with the given "shape". PAM stores rectangular
		 * data.
		 *
		 * DATA format expected by PAM
		 * V = {v_1_1_1, v_1_1_2, ..., v_1_1_Z, v_1_2_1, v_1_2_2, ... v_1_2_N, ... v_1_X_1, ..., v_1_X_N, v_2_1_1, ..., v_2_1_2, ..., v_2_X_N, ..., v_Y_X_N}
		 * 
		 * It's slightly annoying, as the "colour channel" information, the Z-dimension, is in the fastest varying index. 
		 * The idea is that every V_y_x = {v_y_x_1, ..., v_y_x_Z} is a "Tuple" of values (for example the RGB values
		 * in a colour image), and that the data "V" is a set of Tuples. 
		 *
		 * PAM format
		 *
		 * ----------
		 * P7
		 * WIDTH X
		 * HEIGHT Y
		 * DEPTH Z
		 * MAXVAL N
		 * TUPLTYPE S
		 * ENDHDR
		 * V
		 * ----------
		 * X - an ascii number specifying the width of the image
		 * Y - an ascii number specifying the height of the image
		 * Z - an ascii number specifying the number of "colour channels" each pixel has
		 * N - The largest value in the image (maximum of 65535), each value in a tuple is just wide enough to hold the value N, and no larger
		 * S - A string that tells the user how to interpret the Tuples, offical ones are "BLACKANDWHITE", "GRAYSCALE", "RGB", the optional tag 
		 *     "_ALPHA" can be added to each of these to denote the addition of an opacity channel in the Tuple.
		 * V - The image data as the binary value of an unsigned integer with the most significant byte first, has M bytes per value, where
		 *     M is the number of bytes needed to represent the largest value N.
		 *
		*/
		assert(shape.size() > 0);
		assert(sizeof(VALUE_TYPE) <=2);

		struct PAM_header {
			size_t width, height, depth, maxval;
			std::string tupltype;
		} pam_header;


		pam_header.width = shape[0];
		pam_header.height = (shape.size() > 1)? shape[1] : 1;
		pam_header.depth = (shape.size() > 2)? shape[2] : 1;
		pam_header.maxval = (VALUE_TYPE)(-1); // largest size holdable
		pam_header.tupltype = TUPLE_TYPE.value;


		// Normalise values in array
		nan_to_num_inplace(a);
		T min_value = min(a);
		subtract_inplace(a, min_value);

		T max_value = max(a);


		std::vector<VALUE_TYPE> b(a.size());

		for(size_t i=0; i<a.size(); ++i){
			b[i] = static_cast<VALUE_TYPE>((pam_header.maxval * a[i])/max_value);
			if (is_bigendian){
				b[i] = byteswap(b[i]);
			}
		}

		/*
		// Draw crosshair for testing purposes, starts at top-left corner
		size_t i1 = pam_header.width * 0.45;  // fraction of width at which to draw vertical line
		size_t i2 = pam_header.height * 0.55; // fraction of height at which to draw horizontal line
		for (size_t i=0; i<b.size(); ++i){
			if ((i%pam_header.width) == i1){
				b[i] = is_bigendian ? byteswap(static_cast<VALUE_TYPE>(pam_header.maxval)) : pam_header.maxval ;
			}
			else if (i/pam_header.width == i2){
				b[i] = is_bigendian ? byteswap(static_cast<VALUE_TYPE>(pam_header.maxval)) : pam_header.maxval ;
			}
		}
		*/

		std::ofstream fs(fpath, std::ios::out | std::ios::binary);

		fs << "P7\n";
		fs << "WIDTH " << pam_header.width << "\n";
		fs << "HEIGHT " << pam_header.height << "\n";
		fs << "DEPTH " << pam_header.depth << "\n";
		fs << "MAXVAL " << pam_header.maxval << "\n";
		fs << "TUPLTYPE " << pam_header.tupltype <<"\n";
		fs << "ENDHDR\n";
		fs.write(static_cast<char*>(static_cast<void*>(b.data())), b.size() * sizeof(VALUE_TYPE));
		fs.close();


	}

}







#endif //__DATA_UTILS_INCLUDED__
