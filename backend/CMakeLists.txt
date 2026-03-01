cmake_minimum_required(VERSION 3.20)
project(cv_backend VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type defaults to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
find_package(Threads REQUIRED)

# cpp-httplib (header-only, fetched via FetchContent)
include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.15.3
)
FetchContent_MakeAvailable(httplib)

# nlohmann/json (header-only)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/utils/ImageUtils.cpp
    src/processing/NoiseProcessor.cpp
    src/processing/FilterProcessor.cpp
    src/processing/EdgeDetector.cpp
    src/processing/HistogramProcessor.cpp
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
)

target_link_libraries(cv_backend
    PRIVATE
        ${OpenCV_LIBS}
        Threads::Threads
        httplib::httplib
        nlohmann_json::nlohmann_json
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
