#ifndef __STR_PRINTF_INCLUDED__
#define __STR_PRINTF_INCLUDED__

#include <string>
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <vector>
#include <span>

// SPECIFIC TYPE OUTPUT
std::ostream& operator<<(std::ostream& os, const std::byte& a);
std::ostream& operator<<(std::ostream& os, const uint8_t& a);
std::ostream& operator<<(std::ostream& os, const uint8_t* p); // NOTE: this might make printing char* strings wierd

// SPAN OUTPUT
template <class T>
std::ostream& operator<<(std::ostream& os, const std::span<T>& a){
	// Displays a vector as follows
	// Span<T>{size() = X, data = Y}[...]

	size_t N = a.size();

	auto old_flags = os.flags();
	auto old_precision = os.precision();
	auto old_width = os.width();

	os.setf(std::ios_base::scientific, std::ios_base::floatfield); 
	os.setf(std::ios_base::boolalpha | std::ios_base::uppercase);
	os.precision(6);
	os.width(12);

	


	if (&os == &std::cout){
		os << "Span<" << typeid(T).name() << ">{";
		os << "size() = " << N;
		os << ", data() = " << (!a.empty() ? a.data() : nullptr);
		os <<"}";

		if (!a.empty()){
			constexpr size_t M = 3; // will print 3x this number of elements in short version

			//os << "\n";
			os << "[" << a[0];

			if (N <= M*3){
				for (size_t i=1; i<N; ++i){
					os << ", " << a[i];
				}
			} else {
				for (size_t i = 1; i < M; ++i){
					os << ", " << a[i];
				}
				os << ", ...";
				for (size_t i = (N/2 - M/2); i < (N/2 + M - M/2); ++i){
					os << ", " << a[i];
				}
				os << ", ...";
				for (size_t i = N-M; i < N; ++i){
					os << ", " << a[i];
				}
			}
			os << "]";
		}
	}
	else{
		os << "[" << a[0];
		for (int i=1; i<5; ++i){
			os << ", " << a[i];
		}
		os << "]";
	}

	os.flags(old_flags);
	os.precision(old_precision);
	os.width(old_width);



	return(os);

}


// ARRAY OUTPUT

template <class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& a){
	// Displays a vector as follows
	// Vector<T>{size() = X, data = Y}

	size_t N = a.size();

	auto old_flags = os.flags();
	auto old_precision = os.precision();
	auto old_width = os.width();

	os.setf(std::ios_base::scientific, std::ios_base::floatfield); 
	os.setf(std::ios_base::boolalpha | std::ios_base::uppercase);
	os.precision(6);
	os.width(12);

	


	if (&os == &std::cout){
		os << "Vector<" << typeid(T).name() << ">{";
		os << "size() = " << N;
		if (!a.empty()){
			if constexpr(std::is_same<T,bool>::value){
				os << ", data() = non-contigous boolean values ";
			} else {
				os << ", data() = " << a.data();
			}
		}
		os <<"}";

		if (!a.empty()){
			constexpr size_t M = 3; // will print 3x this number of elements in short version

			//os << "\n";
			os << "[" << a[0];

			if (N <= M*3){
				for (size_t i=1; i<N; ++i){
					os << ", " << a[i];
				}
			} else {
				for (size_t i = 1; i < M; ++i){
					os << ", " << a[i];
				}
				os << ", ...";
				for (size_t i = (N/2 - M/2); i < (N/2 + M - M/2); ++i){
					os << ", " << a[i];
				}
				os << ", ...";
				for (size_t i = N-M; i < N; ++i){
					os << ", " << a[i];
				}
			}
			os << "]";
		}
	}
	else{
		os << "[" << a[0];
		for (int i=1; i<5; ++i){
			os << ", " << a[i];
		}
		os << "]";
	}

	os.flags(old_flags);
	os.precision(old_precision);
	os.width(old_width);



	return(os);

}


// print to an output stream
// base function
void _oprintf(std::ostream& os, const char* lineContinue, const char* fmt);

// recursive variadic function
template <class T, class ...Types>
void _oprintf(std::ostream& os, const char* lineContinue, const char* fmt, T&& value, Types&& ...args) {
	for (; *fmt != '\0'; fmt++) {
		if (*fmt == '%') {
			os << value;
			_oprintf(os, lineContinue, fmt + 1, args...);
			return;
		}
		else if (*fmt == '\n') {
			os << std::endl << lineContinue;
		}
		else {
			os << *fmt;
		}
	}
}


// print to a string
// base function
std::string _sprintf(const char* fmt);

// recursive variadic function
template <class T, class... Ts>
std::string _sprintf(const char* fmt, T&& val, Ts&& ... args) {
	std::stringstream ss;
	for (; *fmt != '\0'; fmt++) {
		if (*fmt == '%') {
			ss << val;
			ss << _sprintf(fmt + 1, args...);
			return(ss.str());
		}
		else if (*fmt == '\n') {
			ss << std::endl; // new lines
		}
		else {
			ss << *fmt;
		}
	}
	return(ss.str());
}

// print to stdout
// base function
void _printf(const char* fmt);

// recursive variadic function
template <class T, class... Ts>
void _printf(const char* fmt, T&& val, Ts&& ... args) {
	for (; *fmt != '\0'; fmt++) {
		if (*fmt == '%') {
			std::cout << val;
			_printf(fmt + 1, args...);
			return;
		}
		else if (*fmt == '\n') {
			std::cout << std::endl ; // new lines
		}
		else {
			std::cout << *fmt;
		}
	}
	std::cout << *fmt;
}

#endif //__STR_PRINTF_INCLUDED__
