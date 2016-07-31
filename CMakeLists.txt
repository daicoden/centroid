cmake_minimum_required(VERSION 3.5)
project(centroid)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES sbuf.c sbuf.h sockio.c sockio.h sockserv.c sockserv.h centroid.h centroid.c main.c)
add_executable(centroid ${SOURCE_FILES})
