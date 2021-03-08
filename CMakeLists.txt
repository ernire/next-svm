cmake_minimum_required(VERSION 3.12)
project(next-svm)

set(CMAKE_CXX_STANDARD 17)
#add_executable(DEEP svm_light_to_bin.cpp)
#add_executable(next-svm-light-to-bin source/svm_light_to_bin.cpp source/next_svm_io.cpp source/next_svm_io.h
#        source/test_util.cpp source/test_util.h)
#add_executable(next-svm-predict source/next_svm_predict.cpp source/next_svm_predict.h source/next_svm_io.cpp
#        source/next_svm_io.h source/test_util.cpp source/test_util.h source/pisvm.cpp source/pisvm.h)
add_executable(next-svm source/main.cpp)