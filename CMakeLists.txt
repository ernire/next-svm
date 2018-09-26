cmake_minimum_required(VERSION 3.12)
project(next-svm)

set(CMAKE_CXX_STANDARD 17)
#add_executable(DEEP svm_light_to_bin.cpp)
add_executable(next-svm-predict source/hpc_svm_predict.cpp source/hpc_svm_predict.h)
add_executable(next-svm-light-to-bin source/svm_light_to_bin.cpp source/next_svm_data.cpp source/next_svm_data.h)