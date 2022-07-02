cmake_minimum_required(VERSION 3.16)
project(simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
add_executable(code main.cpp)
