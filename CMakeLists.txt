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

find_package(cxxopts 2.2 REQUIRED)
find_package(pugixml 1.11 REQUIRED)
find_package(libzippp 3.0 REQUIRED)

# Minimum version of cpr must be 1.6.2 and
# it must be compiled with the SSL back-end.
find_path(CPR_INCLUDE NAMES cpr/cpr.h cpr.h REQUIRED)
find_library(CPR_LIB cpr REQUIRED)

find_package(PkgConfig REQUIRED)
# Using IMPORTED_TARGET to create a target
# that will be passed to target_link_libraries.
pkg_check_modules(FCLI REQUIRED IMPORTED_TARGET fcli>=1.2)

set(INCLUDE_DIRS
  ${CPR_INCLUDE}
  include/
)

set(LIBS
  stdc++fs
  cxxopts::cxxopts
  pugixml::pugixml
  libzippp::libzippp
  ${CPR_LIB}
  PkgConfig::FCLI
)

# -------------- #
# Implementation #
# -------------- #

set(SOURCES
  /dev/null
)

# Compile the implementation for both apm and test executables.
add_library(impl OBJECT ${SOURCES})
target_include_directories(impl PUBLIC ${INCLUDE_DIRS})
# Using INTERFACE as libraries must be linked with
# final executables, not with the object library.
target_link_libraries(impl INTERFACE ${LIBS})

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
  test/main.cpp
)

if(BUILD_TESTING)
  find_package(doctest 2.4 REQUIRED)
  add_executable(test ${TEST_SOURCES})
  target_link_libraries(test PRIVATE impl doctest::doctest)
endif()
