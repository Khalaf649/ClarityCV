cmake_minimum_required(VERSION 3.20)
project(cv_backend VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
find_package(Threads REQUIRED)

# nlohmann/json — installed via apt (nlohmann-json3-dev)
find_package(nlohmann_json REQUIRED)

# cpp-httplib — single header downloaded via wget in Dockerfile
# lives at /usr/local/include/httplib.h — no find_package needed

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/utils/ImageUtils.cpp
    src/utils/FourierUtils.cpp 
    src/processing/NoiseProcessor.cpp
    src/processing/FilterProcessor.cpp
    src/processing/EdgeDetector.cpp
    src/processing/HistogramProcessor.cpp
    src/processing/ThresholdProcessor.cpp
    src/processing/FrequencyProcessor.cpp
    src/processing/HybridProcessor.cpp
    src/server/Server.cpp
    src/server/Router.cpp
)

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------

add_executable(cv_backend ${SOURCES})
target_include_directories(cv_backend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
        /usr/local/include
)

target_link_libraries(cv_backend
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        nlohmann_json::nlohmann_json
        ssl
        crypto
)

target_compile_definitions(cv_backend
    PRIVATE
        CPPHTTPLIB_OPENSSL_SUPPORT
)

target_compile_options(cv_backend
    PRIVATE
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
        $<$<CONFIG:Debug>:-g -Wall -Wextra -Wpedantic>
)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
install(TARGETS cv_backend DESTINATION bin)