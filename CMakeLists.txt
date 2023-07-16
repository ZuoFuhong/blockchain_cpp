cmake_minimum_required(VERSION 3.18.2)
project(blockchain_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++17")
set(INC_DIR /usr/local/include)
set(LINK_DIR /usr/local/lib)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})

add_executable(blockchain_cpp main.cc)
target_link_libraries(blockchain_cpp pthread)
