# lp-recognizer
An Automatic License Plate Recognition tool based on the ultimateALPR-SDK [0].

## Building && Running it on Ubuntu 20.04
Please find the info for building/using ultimateALPR-SDK at [0]. It applies to this tool as well.

1) Clone the ultimateALPR:

       $ git clone https://github.com/DoubangoTelecom/ultimateALPR-SDK.git

2) Download the tensorflow and copy it into the `binaries/linux/x86_64` folder (https://github.com/DoubangoTelecom/ultimateALPR-SDK/blob/master/samples/c%2B%2B/README.md#linux).

       $ tar -C ${path-to-the-ultalpr}/ultimateALPR-SDK/binaries/linux/x86_64/ -xf ~/Downloads/libtensorflow_r1.14_cpu+gpu_linux_x86-64.tar.gz

3) Set up build of the lp-recognizer:

       $ mkdir build && cd build
       $ cmake ../lp-recognizer/ -DCMAKE_C_FLAGS="-L${path-to-the-ultalpr}/ultimateALPR-SDK/binaries/linux/x86_64 -I${path-to-the-ultalpr}/ultimateALPR-SDK/c++ -I/${path-to-the-ultalpr}/ultimateALPR-SDK/samples/c++" -DCMAKE_CXX_FLAGS="-L/${path-to-the-ultalpr}/ultimateALPR-SDK/binaries/linux/x86_64 -I${path-to-the-ultalpr}/ultimateALPR-SDK/c++ -I${path-to-the-ultalpr}/ultimateALPR-SDK/samples/c++"
       $ make

4) Running the lp-recognizer:

Before we run the tool install openCL library:

       $ sudo apt install ocl-icd-opencl-dev
       $ export LD_LIBRARY_PATH=${path-to-the-ultalpr}/ultimateALPR-SDK/binaries/linux/x86_64
       $ cd ../examples/
       $ ../build/lp-recognizer --assets ${path-to-the-ultalpr}/ultimateALPR-SDK/assets

The server expects just paths to the images to be processed. The client will recieve a JSON object that containts the outcome of using the [0] library.

[0] https://github.com/DoubangoTelecom/ultimateALPR-SDK
