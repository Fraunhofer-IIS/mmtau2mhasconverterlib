FetchContent_MakeAvailable(ilo mmtisobmff mmtmhasparserlib)

configure_file (
  "${PROJECT_SOURCE_DIR}/src/mmtau2mhasconverterlib_config.h.in"
  "${PROJECT_SOURCE_DIR}/include/mmtau2mhasconverterlib/version.h"
)

add_library(mmtau2mhasconverterlib STATIC
  ${PROJECT_SOURCE_DIR}/include/mmtau2mhasconverterlib/converter.h
  ${PROJECT_SOURCE_DIR}/include/mmtau2mhasconverterlib/converter_config.h
  ${PROJECT_SOURCE_DIR}/include/mmtau2mhasconverterlib/directory_converter.h
  ${PROJECT_SOURCE_DIR}/include/mmtau2mhasconverterlib/file_converter.h
  ${PROJECT_SOURCE_DIR}/include/mmtau2mhasconverterlib/log_redirect.h
  ${PROJECT_SOURCE_DIR}/include/mmtau2mhasconverterlib/version.h
  logging.h
  converter.cpp
  converter_helpers.cpp
  converter_helpers.h
  converter_mha.cpp
  converter_mha.h
  converter_mhm.cpp
  converter_mhm.h
  directories.cpp
  directories.h
  directory_converter.cpp
  file_converter.cpp
  file_converter_pimpl.cpp
  file_converter_pimpl.h
  helpers.cpp
  helpers.h
)

target_compile_features(mmtau2mhasconverterlib PUBLIC cxx_std_11)
set_target_properties(mmtau2mhasconverterlib PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(mmtau2mhasconverterlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(mmtau2mhasconverterlib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mmtau2mhasconverterlib PUBLIC ilo mmtisobmff mmtmhasparserlib)
