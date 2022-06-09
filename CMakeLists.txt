cmake_minimum_required(VERSION 3.8)

project(wadtools)

add_library(
    common STATIC
    src/common.cpp
    src/lump.cpp
    src/wadfile.cpp
)

target_compile_features(common PRIVATE cxx_std_17)
target_include_directories(common PRIVATE stb/)

add_executable(unwad src/unwad.cpp)
target_link_libraries(unwad PRIVATE common)
target_compile_features(unwad PRIVATE cxx_std_17)
