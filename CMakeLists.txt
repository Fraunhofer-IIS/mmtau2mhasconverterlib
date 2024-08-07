cmake_minimum_required(VERSION 3.16)
include(FetchContent)

include_guard()

project(mmtau2mhasconverterlib VERSION 1.0.0)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(mmtau2mhasconverterlib_BUILD_BINARIES OFF CACHE BOOL "Build demo executables")
set(mmtau2mhasconverterlib_BUILD_DOC      OFF CACHE BOOL "Build doxygen doc")

FetchContent_Declare(
  ilo
  GIT_REPOSITORY https://github.com/Fraunhofer-IIS/ilo.git
  GIT_TAG        r1.0.0
)

FetchContent_Declare(
  mmtisobmff
  GIT_REPOSITORY https://github.com/Fraunhofer-IIS/mmtisobmff.git
  GIT_TAG        r1.0.0
)

FetchContent_Declare(
  mmtaudioparser
  GIT_REPOSITORY https://github.com/Fraunhofer-IIS/mmtaudioparser.git
  GIT_TAG        r1.0.0
)

FetchContent_Declare(
  mmtmhasparserlib
  GIT_REPOSITORY https://github.com/Fraunhofer-IIS/mmtmhasparserlib.git
  GIT_TAG        r1.0.0
)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "armv7.*")
  # (At least) on 32-bit ARMv7, the POSIX file functions are 32-bit, but we rely on the 64-bit versions.
  # For any 64-bit platform, the file functions default to their 64-bit versions anyway.
  # See https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
  add_compile_options(-D_FILE_OFFSET_BITS=64)
endif()

add_subdirectory(src)

if(mmtau2mhasconverterlib_BUILD_BINARIES)
  add_subdirectory(demo)
endif()

if(mmtau2mhasconverterlib_BUILD_DOC)
  add_subdirectory(doc)
endif()
