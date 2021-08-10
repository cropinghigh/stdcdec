cmake_minimum_required(VERSION 3.5)

project(stdcdec LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
add_executable(stdc_demod stdc_demod.cpp)
add_executable(stdc_decoder stdc_decoder.cpp)
add_executable(stdc_parser stdc_parser.cpp)
target_link_libraries(stdc_demod inmarsatc_demodulator asound audiofile)
target_link_libraries(stdc_decoder inmarsatc_decoder)
target_link_libraries(stdc_parser inmarsatc_parser)
