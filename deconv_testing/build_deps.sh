
set -o errexit -o pipefail -o nounset

repos_dir="${REPOS_DIR:-"${HOME}/repos"}"
emscripten_repo="${repos_dir}/emsdk"
THIS_DIR=$(readlink -f $(dirname ${BASH_SOURCE}))
THIS_PREFIX="$(readlink -f ${THIS_DIR})"
THIS_INCLUDE_DIR="$(readlink -f ${THIS_DIR}/include)"
THIS_LIB_DIR="$(readlink -f ${THIS_DIR}/lib)"
THIS_ARCHIVE_DIR="$(readlink -f ${THIS_DIR}/archive)"


##############################
# Activate emsdk environment #
##############################

set +o nounset
if [[ -z "${EMSDK}" ]]; then
	export EMSDK_QUIET=1 # suppress EMSDK source output
	source ${emscripten_repo}/emsdk_env.sh
fi
set -o nounset


mkdir -p ${THIS_INCLUDE_DIR} ${THIS_LIB_DIR} ${THIS_ARCHIVE_DIR}


export CC=~/repos/emsdk/upstream/emscripten/emcc
#export CFLAGS="-I${THIS_INCLUDE_DIR} -L${THIS_LIB_DIR}"
export CPPC=~/repos/emsdk/upstream/emscripten/em++
export prefix=${THIS_PREFIX}
#export LIBS="z"

#export LT_SYS_LIBRARY_PATH="${THIS_LIB_DIR}"

#declare -a REQS=("eigen" "cfitsio" "fftw" "netpbm")
#declare -a REQS=("eigen" "cfitsio" "fftw")
#declare -a REQS=("netpbm")
#declare -a REQS=("zlib")
declare -a REQS=("zlib" "libjpeg" "libtiff" "eigen" "fftw")

declare -A REQ_REMOTES=(
	[eigen]="https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz"
	[cfitsio]="https://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio-4.5.0.tar.gz"
	[fftw]="https://www.fftw.org/fftw-3.3.10.tar.gz"
	[netpbm]="https://sourceforge.net/projects/netpbm/files/super_stable/10.86.43/netpbm-10.86.43.tgz/download"
	[zlib]="https://zlib.net/zlib-1.3.1.tar.gz"
	[libjpeg]="http://www.ijg.org/files/jpegsrc.v9b.tar.gz"
	[libtiff]="http://download.osgeo.org/libtiff/tiff-4.7.0.tar.gz"

)

declare -A REQ_SOURCES=(
	[libjpeg]="jpeg-9b"
)

declare -A REQ_ACTIONS=(
	[eigen]="cp -r Eigen ${THIS_INCLUDE_DIR};"
	#[cfitsio]="configure --disable-curl --without-zlib-check --without-fortran;  make; install;"
	[cfitsio]="configure --disable-curl --without-fortran;  make; install;"
	[fftw]="configure --disable-fortran; make; install;"
	[netpbm]="configure; make; install;"
	[zlib]="./configure; make; install;"
	[libjpeg]="configure; make; install;"
	[libtiff]="configure; make; install;"
)


shopt -s extglob

for REQ in "${REQS[@]}"; do
	REQ_REMOTE="${REQ_REMOTES[$REQ]}"
	REQ_ARCHIVE="${REQ_REMOTE%gz*}gz"
	REQ_ARCHIVE="${REQ_ARCHIVE##*/}"

	set +o nounset
	if [ -z "${REQ_SOURCES[$REQ]}" ]; then
		REQ_SRC="${REQ_ARCHIVE%.@(tar.gz|tgz)*}"
	else
		REQ_SRC=${REQ_SOURCES[$REQ]}
	fi
	set -o nounset

	echo "REQ=${REQ}"
	echo "REQ_REMOTE=${REQ_REMOTE}"
	echo "REQ_ARCHIVE=${REQ_ARCHIVE}"
	echo "REQ_SRC=${REQ_SRC}"



	cd ${THIS_ARCHIVE_DIR}
	
	if [[ ! -e "${THIS_ARCHIVE_DIR}/${REQ_ARCHIVE}" ]]; then
		wget ${REQ_REMOTE} -O ${REQ_ARCHIVE}
	fi
	
	if [[ ! -e "${THIS_ARCHIVE_DIR}/${REQ_SRC}" ]]; then
		tar -xzf ${REQ_ARCHIVE}
	fi


	cd ${REQ_SRC}

	IFS_PREV="${IFS}"
	IFS=';'
	ACTIONS=(${REQ_ACTIONS[$REQ]})
	IFS="${IFS_PREV}"


	for ACTION in "${ACTIONS[@]}"; do
		# remove whitespace from ACTION
		ACTION=${ACTION%%*([[:space:]])}
		ACTION=${ACTION##*([[:space:]])}
		echo "ACTION='${ACTION}'"
		CMD=(${ACTION})
		echo "CMD[@]=${CMD[@]}"

		case "${CMD[0]}" in
			"configure")
				#if [[ ! -e "./config.status" ]]; then
					emconfigure ./configure ${CMD[@]:1} --prefix=${THIS_PREFIX} CC=${CC}
				#fi
				;;
			"make")
				emmake make ${CMD[@]:1}
				;;
			"install")
				emmake make install
				;;
			*)
				(${CMD[@]})
				;;
		esac
	done

	continue # DEBUGGING

done
shopt -u extglob


exit
