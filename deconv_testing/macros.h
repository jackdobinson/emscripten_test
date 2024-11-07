#ifndef __MACROS_INCLUDED__
	#define __MACROS_INCLUDED__

#include <cstring>
// I should put all macros in here if I can
// that way I only have to include one header file.

#define __SLN_DIR__ ""
#define SRC_FILE (&(__FILE__[strlen(__SLN_DIR__)]))
#ifdef _MSC_VER
	#define __PRETTY_FUNCTION__ __FUNCSIG__ 
#endif //_MSC_VER

// utility macros
#define _I(A) A
#define STR(A) STR_I(A)
#define STR_I(A) #A
#define CAT(A,B) CAT_I(A,B)
#define CAT_I(A,B) A##B

#endif //__MACROS_INCLUDED__
