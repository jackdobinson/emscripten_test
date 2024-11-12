# Makefile for deconvolution C++ implementation
#
# Example Command
# ./deconv ~/Documents/projects/deconvolution_tutorial/data/test_rebin.fits[1] ~/Documents/projects/deconvolution_tutorial/data/fit_example_psf_000.fits[0]


#CXX=g++-11
#CXX=g++-12
CXX=em++

LDIRS=-L ~/usr/lib -L ./lib
IDIRS=-I ~/Documents/code/cpp_code/include -I ~/usr/include -I ./include

EXPORT_FUNCS=          \
	"_malloc"          \
	"_free"            \
	"_main"

#	"load_image_rgba"  \
#	"_TIFFOpen"        \
#	"_TIFFClose"       \
#	"_TIFFGetField"    \
#	"_TIFFReadRGBAImage"        \
#	"_TIFFReadRGBAImageOriented"\
#	"_TIFFSetDirectory"         \
#	"_TIFFCurrentDirectory"     \
#	"_TIFFReadDirectory"        \
#	"__TIFFmalloc"              \
#	"__TIFFfree"                \
#	"_GetField"                 \
#	"FS"               \

empty:=
space:= $(empty) $(empty)
comma:= ,


#CXXFLAGS=-O3 $(LDIRS) $(IDIRS) -lcfitsio -lfftw3 -lm -lnetpbm -std=gnu++20 -Wshadow
CXXFLAGS=                       \
	-sASYNCIFY                  \
	-sASSERTIONS=2              \
	-sSTACK_OVERFLOW_CHECK=2    \
	-sENVIRONMENT=web           \
	-sALLOW_MEMORY_GROWTH=1     \
	-sEXPORTED_FUNCTIONS=[$(subst $(space),$(comma),$(EXPORT_FUNCS))] \
	-sNO_DISABLE_EXCEPTION_CATCHING \
	-mnontrapping-fptoint       \
	-O3                         \
	$(LDIRS)                    \
	$(IDIRS)                    \
	-lfftw3                     \
	-lm                         \
	-lembind                    \
	-lz                         \
	-ljpeg                      \
	-ltiff                      \
	-std=gnu++20                \
	-Wshadow


#	-fexceptions                \

deconv.js : *.cpp *.h *.hpp
	$(CXX) deconv.cpp str_printf.cpp data_utils.cpp fft.cpp storage.cpp tiff_helper.cpp main.cpp -o deconv.js $(CXXFLAGS)

clean:
	rm -f deconv
