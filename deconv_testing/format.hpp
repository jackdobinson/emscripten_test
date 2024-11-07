#include <string>
#include <iostream>
#include <sstream>
#include <exception>
#include <ctype.h>
#include <stdlib.h>


namespace text {

	template<class T>
	size_t str_to_int(const char* ptr, T& value, int base=10){
		char* endptr = (char*)ptr;
		value = strtol(ptr, &endptr, base);
		return endptr - ptr;
	}

	template <class T>
	std::string do_format(const char* fmt_spec, size_t n, T value){
		std::stringstream ss;
		std::ios_base::fmtflags new_ff=std::ios_base::scientific;

		//std::cout << "########";
		//std::cout << std::string(fmt_spec, n);
		//std::cout << std::flush;

		// Should be of format "{:...}"
		if (fmt_spec[1] != ':'){
			throw std::runtime_error("Format spec must start with a colon ':'");
		}

		size_t i=0;
		size_t precision;  
		size_t n_consumed;
		size_t width;
		while(i<n){
			switch (fmt_spec[i]) {
				case ':': // ignore colons
				case '{': // ignore curly brackets
				case '}':
					i++;
					break;
				case '<':
					new_ff |= std::ios_base::left;
					i++;
					break;
				case '>':
					new_ff |= std::ios_base::right;
					i++;
					break;
				case '^':
					new_ff |= std::ios_base::internal;
					i++;
					break;
				case '+':
					new_ff |= std::ios_base::showpos;
					i++;
					break;
				case '-':
					new_ff &= ~std::ios_base::showpos; // unset show "+" for positive numbers
					i++;
					break;
				case ' ':
					new_ff &= ~std::ios_base::showpos; // unset show "+" for positive numbers
					if (value >= +0) { // inset space before +ve number
						ss << ' ';
					}
					i++;
					break;
				case '#':
					new_ff |= std::ios_base::showbase | std::ios_base::showpoint;
					i++;
					break;
				case '0':
					ss.fill('0');
					i++;
					break;
				case '.':
					// must be followed by a number
					if (i+1 == n){
						throw std::runtime_error("In format specifier, dot '.' must be followed by a number to specify precision");
					}
					n_consumed = str_to_int(fmt_spec+i+1, precision);
					ss.precision(precision);

					i += 1+n_consumed;
					break;
				case 'X':
					new_ff |= std::ios_base::uppercase;
				case 'x':
					new_ff |= std::ios_base::hex;
					i++;
					break;
				case 'O':
					new_ff |= std::ios_base::uppercase;
				case 'o':
					new_ff |= std::ios_base::oct;
					i++;
					break;
				case 'd':
					new_ff |= std::ios_base::dec;
					i++;
					break;
				case 'f':
					new_ff |= std::ios_base::fixed;
					i++;
					break;
				default:
					// if not one of these, must be the width field
					n_consumed = str_to_int(fmt_spec+i, width);
					ss.width(width);
					i += n_consumed;

			}


		}
		ss.flags(new_ff);
		ss << value;
		return ss.str();
	}

	std::string format (const char* fmt){
		std::stringstream ss;
		ss << fmt;
		return(ss.str());
	}

	template <class T, class... Ts>
	std::string format(const char* fmt, T value, Ts ...args){
		std::stringstream ss;
		int depth = 0;
		const char* fmt_spec_start=nullptr;
		const char* fmt_lookahead = nullptr;
		size_t fmt_spec_n=0;

		const char* fmt_start = fmt;

		for (; *fmt != '\0'; fmt++) {

			// Only work on the smallest enclosed brackets
			if (*fmt == '{') {
				fmt_spec_start = fmt;

				for(fmt_lookahead = fmt+1; (*fmt_lookahead != '\0') | (*fmt_lookahead != '}'); fmt_lookahead++){
					if (*fmt_lookahead == '{'){
						ss << *fmt;
						break;
					}

					if (*fmt_lookahead == '}'){
						fmt_spec_n = fmt_lookahead - fmt_spec_start;
						ss << do_format(fmt_spec_start, fmt_spec_n, value);
						fmt += fmt_spec_n+1;
					}
				}
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

}


