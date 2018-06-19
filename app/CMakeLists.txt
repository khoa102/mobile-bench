# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add_library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

add_library( # Specifies the name of the library.
              native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/cpp/native-lib.cpp )

set_target_properties(native-lib PROPERTIES LINKER_LANGUAGE CXX)

# Library for doing GPU vectorAdd
add_library( # Specifies the name of the library.
              vectorAdd-lib

              # Sets the library as a shared library.
              SHARED

              # Provides a relative path to your source file(s).
              src/cpp/vectorAdd.cpp )

set_target_properties(vectorAdd-lib PROPERTIES LINKER_LANGUAGE CXX)

# Library for Benchmark
add_library(# Specifies the name of the library.
            benchmark-lib

            # Sets the library as a shared library.
            SHARED

            # Provides a relative path to your source file(s).
            src/cpp/benchmark/benchmark.cpp )

set_target_properties(benchmark-lib PROPERTIES LINKER_LANGUAGE CXX)

# Library for Event
add_library(# Specifies the name of the library.
            event-lib

            # Sets the library as a shared library.
            SHARED

            # Provides a relative path to your source file(s).
            src/cpp/benchmark/Event.cpp )

set_target_properties(event-lib PROPERTIES LINKER_LANGUAGE CXX)

# Library for BusSpeedDownload
add_library(# Specifies the name of the library.
            busSpeedDownload-lib

            # Sets the library as a shared library.
            SHARED

            # Provides a relative path to your source file(s).
            src/cpp/benchmark/SHOC-level0/BusSpeedDownload.cpp )

set_target_properties(busSpeedDownload-lib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(busSpeedDownload-lib event-lib)
target_link_libraries(benchmark-lib busSpeedDownload-lib)

# Library for BusSpeedReadback
add_library(# Specifies the name of the library.
            busSpeedReadback-lib

            # Sets the library as a shared library.
            SHARED

            # Provides a relative path to your source file(s).
            src/cpp/benchmark/SHOC-level0/busSpeedReadback.cpp )

set_target_properties(busSpeedReadback-lib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(busSpeedReadback-lib event-lib)
target_link_libraries(benchmark-lib busSpeedReadback-lib)

# Library for doing GPU matrixMul
add_library( # Specifies the name of the library.
              matrixMul-lib

              # Sets the library as a shared library.
              SHARED

              # Provides a relative path to your source file(s).
              src/cpp/benchmark/matrixMul/matrixMul.cpp )

set_target_properties(matrixMul-lib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(matrixMul-lib event-lib)
target_link_libraries(benchmark-lib matrixMul-lib)

# Specifies a path to native header files.
include_directories(src/cpp/)
include_directories(src/cpp/benchmark/)
include_directories(src/cpp/benchmark/SHOC-level0/)
include_directories(src/cpp/benchmark/matrixMul/)

# OpenCL library
set(LIBS_DIR ${CMAKE_SOURCE_DIR}/libs/openCL/)
add_library(libOpenCL SHARED IMPORTED)
set_target_properties(libOpenCL PROPERTIES IMPORTED_LOCATION ${LIBS_DIR}/${ANDROID_ABI}/libOpenCL.so)
include_directories(${LIBS_DIR}/include/)

target_link_libraries(vectorAdd-lib libOpenCL)
target_link_libraries(matrixMul-lib libOpenCL)
target_link_libraries(benchmark-lib libOpenCL)
target_link_libraries(event-lib libOpenCL)
target_link_libraries(busSpeedDownload-lib libOpenCL)
target_link_libraries(busSpeedReadback-lib libOpenCL)
target_link_libraries(native-lib libOpenCL)