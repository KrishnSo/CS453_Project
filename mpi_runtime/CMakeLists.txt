cmake_minimum_required(VERSION 3.10)
project(ngs_mpi CXX)

find_package(MPI REQUIRED)

set(CMAKE_CXX_STANDARD 17)

include_directories(include ${MPI_CXX_INCLUDE_PATH})

add_executable(ngs_mpi src/main.cpp)

target_link_libraries(ngs_mpi MPI::MPI_CXX)