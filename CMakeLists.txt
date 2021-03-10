cmake_minimum_required(VERSION 3.14)
project(next-svm)

set(OMP ON)
set(MPI OFF)
set(CUDA OFF)
set(DEBUG ON)
set(GTEST OFF)

set(CMAKE_CXX_STANDARD 20)

if (OMP)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -fno-exceptions")
    add_definitions(-DOMP_ON)
    find_package(OpenMP REQUIRED)
    add_executable(next-svm source/main.cpp source/next_svm.cpp)
    target_link_libraries(next-svm PUBLIC OpenMP::OpenMP_CXX)
else()
    add_executable(next-svm source/main.cpp source/next_svm.cpp)
endif()
if (DEBUG)
    add_definitions(-DDEBUG_ON)
endif()

if (MPI)
    add_definitions(-DMPI_ON)
    find_package(MPI REQUIRED)
    include_directories(${MPI_INCLUDE_PATH})
    SET(CMAKE_CXX_COMPILER "${MPI_CXX_COMPILER}")

    if (OMP)
        set(CMAKE_CXX_STANDARD 20)
        add_definitions(-DOMP_ON)
        find_package(OpenMP REQUIRED)
        set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3 -fno-exceptions")
    endif()

    add_executable(next-svm-mpi main.cpp ${NDB_FILES})
    if(MPI_COMPILE_FLAGS)
        set_target_properties(next-svm-mpi PROPERTIES
                COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
    endif()
    if(MPI_LINK_FLAGS)
        set_target_properties(next-svm-mpi PROPERTIES
                LINK_FLAGS "${MPI_LINK_FLAGS}")
    endif()

    if (OMP)
        target_link_libraries(next-svm-mpi PUBLIC OpenMP::OpenMP_CXX ${MPI_LIBRARIES})
    else()
        set(CMAKE_CXX_FLAGS "-Wall")
        target_link_libraries(next-svm-mpi ${MPI_LIBRARIES})
    endif()
endif()


