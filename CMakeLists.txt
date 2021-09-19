cmake_minimum_required(VERSION 3.18)
project(apm
        VERSION 0.1.0
        DESCRIPTION "Fastest way to build Android applications"
        HOMEPAGE_URL "https://github.com/lem0nez/apm"
        LANGUAGES CXX)

option(BUILD_TESTING "Build unit tests (doctest framework required)" ON)

if(NOT (UNIX AND NOT APPLE))
  message(FATAL_ERROR "Only Linux systems are supported!")
endif()

# Require exact version of the standard library.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------ #
# Dependencies #
# ------------ #

# Set flags for both preprocessor and linker.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(OpenSSL 1.1.1 COMPONENTS Crypto REQUIRED)
find_package(cxxopts 2.2 REQUIRED)
find_package(pugixml 1.11 REQUIRED)
find_package(libzippp 3.0 REQUIRED)

# At least version 1.8 is required.
find_package(JNI REQUIRED)
# Version of the JNI wrapper by Mapbox must be at least 4.0.1.
find_path(JNI_HPP_INCLUDE jni/jni.hpp REQUIRED)

# At least version 1.0.3 of PStreams by Jonathan Wakely is required.
find_path(PSTREAMS_INCLUDE pstreams/pstream.h REQUIRED)

# Minimum version of cpr must be 1.6.2 and
# it must be compiled with the SSL back-end.
find_path(CPR_INCLUDE cpr/cpr.h REQUIRED)
find_library(CPR_LIB cpr REQUIRED)

find_package(PkgConfig REQUIRED)
# Using IMPORTED_TARGET to create a target
# that will be passed to target_link_libraries.
pkg_check_modules(FCLI REQUIRED IMPORTED_TARGET fcli>=1.4)

set(INCLUDE_DIRS
  ${JNI_INCLUDE_DIRS}
  ${JNI_HPP_INCLUDE}
  ${PSTREAMS_INCLUDE}
  ${CPR_INCLUDE}
  include/
)

set(LIBS
  stdc++fs
  Threads::Threads
  OpenSSL::Crypto
  cxxopts::cxxopts
  pugixml::pugixml
  libzippp::libzippp
  ${JNI_LIBRARIES}
  ${CPR_LIB}
  PkgConfig::FCLI
)

# -------------- #
# Implementation #
# -------------- #

set(SOURCES
  src/apm.cpp
  src/config.cpp
  src/jvm.cpp
  src/project.cpp
  src/sdk.cpp
  src/utils.cpp
)

# Compile the implementation for both apm and test executables.
add_library(impl OBJECT ${SOURCES})
target_include_directories(impl PUBLIC ${INCLUDE_DIRS})
# Using INTERFACE as libraries must be linked with
# final executables, not with the object library.
target_link_libraries(impl INTERFACE ${LIBS})
target_compile_definitions(impl PUBLIC APM_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Preserve all debug symbols to improve debugging with GDB.
  target_compile_options(impl PUBLIC -fno-limit-debug-info)
endif()

# -------------- #
# apm executable #
# -------------- #

# Include the install directory variables (CMAKE_INSTALL_<dir>).
include(GNUInstallDirs)

add_executable(apm src/main.cpp)
target_link_libraries(apm PRIVATE impl)
install(TARGETS apm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# --------------- #
# test executable #
# --------------- #

set(TEST_SOURCES
  test/internal/args.cpp
  test/internal/env.cpp
  test/internal/tmp_dir.cpp
  test/main.cpp

  test/general/scope_guard.cpp

  test/apm.cpp
  test/config.cpp
  test/jvm.cpp
  test/project.cpp
  test/sdk.cpp
  test/tmp_file.cpp
  test/utils.cpp
)

if(BUILD_TESTING)
  find_package(doctest 2.4 REQUIRED)
  add_executable(test ${TEST_SOURCES})

  target_include_directories(test PRIVATE test/)
  target_link_libraries(test PRIVATE impl doctest::doctest)
endif()
