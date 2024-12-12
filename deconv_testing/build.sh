

repos_dir="${REPOS_DIR:-"${HOME}/repos"}"
emscripten_repo="${repos_dir}/emsdk"
this_dir=$(readlink -f $(dirname ${BASH_SOURCE}))
wasm_server_pid_file="${this_dir}/wasm_server.pid"
launch_wasm_server_flag="false"
wasm_server_pid=""

set -o errexit -o pipefail -o nounset


echo "${repos_dir}"
echo "${emscripten_repo}"
echo "${this_dir}"

###########################
# Launch WASM test server #
###########################

if [[ -e "${wasm_server_pid_file}" ]]; then
	wasm_server_pid=$(cat ${wasm_server_pid_file})
fi

if [[ -z "${wasm_server_pid}" ]]; then
	# If we don't have a PID then we need to launch the server
	launch_wasm_server_flag="true"
	
elif ps -p "${wasm_server_pid}" &> /dev/null; then
	# We have a PID and it is running, therefore we do not need to launch the server
	launch_wasm_server_flag="false"
else
	# We have a PID but it is not running, therefore we need to launch the server
	launch_wasm_server_flag="true"
fi


echo "########################################"
if [[ "${launch_wasm_server_flag}" == "true" ]]; then
	echo "# Need to start wasm test server       #"
	
	nohup python3 -m http.server &> nohup.out &
	wasm_server_pid=$!
	echo "${wasm_server_pid}" > ${wasm_server_pid_file}
	
	echo "# Wasm test server started.            #"
else
	echo "# Wasm test server is already running. #"
fi
echo "########################################"

echo ""
echo "Wasm test server PID is ${wasm_server_pid}"
echo ""
echo "In your browser, navigate to 'localhost:8000/minimal.html' to see the web page."
echo ""
echo "Use the command \`kill ${wasm_server_pid}\` to stop serving the web page."


##############################
# Activate emsdk environment #
##############################

set +o nounset
if [[ -z "${EMSDK}" ]]; then
	export EMSDK_QUIET=1 # suppress EMSDK source output
	source ${emscripten_repo}/emsdk_env.sh
fi
set -o nounset


############################
# Perform build operations #
############################
echo ""
echo "Starting compilation..."
echo ""

set +o errexit
emmake make
compilation_exit_code=$?
set -o errexit

echo ""
if [[ ${compilation_exit_code} -eq 0 ]]; then
	echo "##########################"
	echo "# Compilation Successful #"
	echo "##########################"
else
	echo "######################"
	echo "# Compilation Failed #"
	echo "######################"
fi