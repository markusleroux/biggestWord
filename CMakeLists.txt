cmake_minimum_required(VERSION 3.19)
project(biggest_word)

set(CMAKE_CXX_STANDARD 20)

find_library(CURL curl REQUIRED HINTS /usr/include/curl)

add_executable(biggest_word main.cpp)

target_link_libraries(biggest_word curl)