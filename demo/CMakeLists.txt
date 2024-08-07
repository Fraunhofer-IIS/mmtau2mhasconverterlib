add_executable(mhatomhm mhatomhm.cpp)
target_link_libraries(mhatomhm mmtau2mhasconverterlib)
target_include_directories(mhatomhm PRIVATE ${PROJECT_SOURCE_DIR}/src)
