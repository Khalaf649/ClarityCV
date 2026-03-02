# rm -rf build/
# mkdir build
cd build
cmake ..   -DOpenCV_INCLUDE_DIRS=/usr/include/opencv4   -DOpenCV_LIBS="/usr/lib/x86_64-linux-gnu/libopencv_core.so;/usr/lib/x86_64-linux-gnu/libopencv_imgproc.so;/usr/lib/x86_64-linux-gnu/libopencv_imgcodecs.so;/usr/lib/x86_64-linux-gnu/libopencv_highgui.so"
make 
./cv_backend