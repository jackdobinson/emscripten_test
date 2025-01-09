#include "str_printf.h"

// SPECIFIC TYPE OUTPUT
std::ostream& operator<<(std::ostream& os, const std::byte& a){
	char buf[5];
	snprintf(buf, 5, "%x", (uint8_t)a);
	os << buf;
	return os;
}

std::ostream& operator<<(std::ostream& os, const uint8_t& a){
	char buf[5];
	snprintf(buf, 5, "%x", a);
	os << buf;
	return os;
}

std::ostream& operator<<(std::ostream& os, const uint8_t* p){
	os << reinterpret_cast<const void*>(p); // NOTE: this might make printing char* strings wierd
	return os;
}


// base function
void _oprintf(std::ostream& os, const char* lineContinue, const char* fmt) {
	os << fmt;
}

// base function
std::string _sprintf(const char* fmt) {
	std::stringstream ss;
	ss << fmt;
	return(ss.str());
}

// base function
void _printf(const char* fmt) {
	std::cout << fmt;
	return;
}

