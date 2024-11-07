
set -o errexit -o pipefail -o nounset


THIS_DIR=$(readlink -f $(dirname ${BASH_SOURCE}))
THIS_PREFIX="$(readlink -f ${THIS_DIR})"
THIS_INCLUDE_DIR="$(readlink -f ${THIS_DIR}/include)"
THIS_LIB_DIR="$(readlink -f ${THIS_DIR}/lib)"
THIS_ARCHIVE_DIR="$(readlink -f ${THIS_DIR}/archive)"

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



if [[ ! -e "${THIS_ARCHIVE_DIR}/eigen-3.4.0.tar.gz" ]]; then
	(
		cd ${THIS_ARCHIVE_DIR} 
	 	wget "https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz" "${THIS_ARCHIVE_DIR}"
	
		tar -xvzf "eigen-3.4.0.tar.gz"
		cp -r "eigen-3.4.0/Eigen" "${THIS_INCLUDE_DIR}/"
	)
fi


if [[ ! -e "${THIS_ARCHIVE_DIR}/cfitsio-4.5.0" ]]; then

	(
		cd "${THIS_ARCHIVE_DIR}"
		wget "https://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio-4.5.0.tar.gz" "${THIS_ARCHIVE_DIR}"
		tar -xvzf "cfitsio-4.5.0.tar.gz"
		cd "cfitsio-4.5.0"
		./configure --prefix="${THIS_PREFIX}"
		make
		make install
	)
fi

if [[ ! -e "${THIS_ARCHIVE_DIR}/fftw-3.3.10" ]]; then
	
	(
		cd "${THIS_ARCHIVE_DIR}"
		wget "https://www.fftw.org/fftw-3.3.10.tar.gz"
		tar -xvzf "fftw-3.3.10.tar.gz"
		cd "fftw-3.3.10"
		./configure --prefix="${THIS_PREFIX}"
		make
		make install
	)
fi


########### TIFF SUPPORT ############
export EMCC_CFLAGS="-O2"
ZLIB_PKGVER=1.2.8
LIBTIFF_PKGVER=4.0.6
LIBJPEG_PKGVER=9b
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# build zlib
wget http://zlib.net/current/zlib-${ZLIB_PKGVER}.tar.gz
tar xf zlib-${ZLIB_PKGVER}.tar.gz
cd zlib-${ZLIB_PKGVER}
emconfigure ./configure
emmake make
cd ..

# build libjpeg
wget http://www.ijg.org/files/jpegsrc.v${LIBJPEG_PKGVER}.tar.gz
tar xf jpegsrc.v${LIBJPEG_PKGVER}.tar.gz
cd jpeg-${LIBJPEG_PKGVER}
emconfigure ./configure
emmake make clean # do not ask me why i have to clean here...
emmake make
cd ..

# # build libtiff
wget http://download.osgeo.org/libtiff/tiff-${LIBTIFF_PKGVER}.tar.gz
tar xzvf tiff-${LIBTIFF_PKGVER}.tar.gz
cd tiff-${LIBTIFF_PKGVER}
# see: https://github.com/kripken/emscripten/issues/662
patch -p0 < ../tif_open.c.patch
patch -p0 < ../tiff.h.patch
emconfigure ./configure \
            --with-zlib-include-dir=${DIR}/zlib-${ZLIB_PKGVER}/ \
            --with-zlib-lib-dir=${DIR}/zlib-${ZLIB_PKGVER}/ \
            --with-jpeg-include-dir=${DIR}/jpeg-${LIBJPEG_PKGVER}/ \
            --with-jpeg-lib-dir=${DIR}/jpeg-${LIBJPEG_PKGVER}/.libs/ \
            --enable-shared
emmake make
cd ..


