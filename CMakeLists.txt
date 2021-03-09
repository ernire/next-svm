cmake_minimum_required(VERSION 3.14)
project(next-svm)

set(OMP ON)
set(MPI OFF)
set(CUDA OFF)
set(DEBUG ON)
set(GTEST OFF)

set(CMAKE_CXX_STANDARD 20)

if (OMP)
#    set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -fno-exceptions")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -fno-exceptions")
    add_definitions(-DOMP_ON)
    find_package(OpenMP REQUIRED)
    add_executable(next-svm source/main.cpp source/next_svm.cpp)
    target_link_libraries(next-svm PUBLIC OpenMP::OpenMP_CXX)
else()
    add_executable(next-svm source/main.cpp source/next_svm.cpp)
endif()


