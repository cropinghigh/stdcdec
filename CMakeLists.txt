cmake_minimum_required(VERSION 3.5)

project(stdcdec LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
add_executable(stdcdec main.cpp)
target_link_libraries(stdcdec inmarsatc asound audiofile)
