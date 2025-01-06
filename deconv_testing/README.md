
# Building #

Run the following scripts:

* "${REPO}/deconv_testing/build_deps.sh" to build the dependencies for the project.

* "${REPO}/deconv_testing/build.sh" to build the project.

And follow their instructions. For more information on the operations performed by the 
script "${REPO}/deconv_testing/build.sh", see the following details.

## Project Build Details ## 

### Compiling ###

Activate the emscripten compiler by navigating into the emscripten repository `~/repos/emsdk` and running `source ./emsdk_env.sh`.

When building, use `emconfigure ./configure` and `emmake make` instead of just "./configure" and "make".

### Running ###

Host a web server for testing. The simplest way is to use python to run a simple server in the desired directory. 

In this repositories main directory, run the command `python3 -m http.server` to start a testing server.

In a web browser, navigate to [the testing html page for the tool](http://0.0.0:8000/deconv_testing/minimal.html)
