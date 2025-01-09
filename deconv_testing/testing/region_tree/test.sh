#!/bin/bash


repos_dir="${REPOS_DIR:-"${HOME}/repos"}"
emscripten_repo="${repos_dir}/emsdk"
this_dir=$(readlink -f $(dirname ${BASH_SOURCE}))
src_dir="${this_dir}/../../"

l_dirs=(
	-L ~/usr/lib 
#	-L ${src_dir}/lib
)
i_dirs=(
	-I ~/Documents/code/cpp_code/include 
	-I ~/usr/include 
	-I ${src_dir}/include 
	-I ${src_dir}
)
cxx_flags=(
	-O3 
	${l_dirs[@]} 
	${i_dirs[@]} 
#	-lfftw3 
#	-lm 
#	-lz 
#	-ljpeg 
#	-ltiff 
	-std=gnu++20
)

g++ -o test_bin  test.cpp ${src_dir}/data_utils.cpp ${src_dir}/str_printf.cpp ${cxx_flags[@]}

compilation_failed=$?

if [ ${compilation_failed} == 1 ]; then
	echo "######################"
	echo "# COMPILATION FAILED #"
	echo "######################"
	exit
else
	echo "########################"
	echo "# COMPILATION COMPLETE #"
	echo "########################"
fi

./test_bin
