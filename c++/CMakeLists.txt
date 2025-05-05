cmake_minimum_required(VERSION 3.10)

project(DataProcessor)

# Force use of system compiler
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_INSTALL_PREFIX "/usr")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;/usr/lib")  

set(GS_COMM ${PROJECT_SOURCE_DIR}/gs_examples_communication)
set(GS_SER ${GS_COMM}/gs_examples_serialization)

set(BUILD_CCSDS ON)

# Set paths for Avro
set(AVRO_CPP_PATH "/usr/include")
set(AVRO_LIBRARY_DIR "/usr/lib/x86_64-linux-gnu")

set(INC_DIR
    "include"  # Include directory within the small project
    "${AVRO_CPP_PATH}"  # Avro include directory
    "${AVRO_CPP_PATH}/avro"  # Avro C++ headers
    "/usr/local/include"  # cppzmq headers
    "/usr/include"  # System headers
)

find_library(AVRO_CPP_LIBRARY avro REQUIRED PATHS ${AVRO_LIBRARY_DIR})

list(APPEND LIBS 
    avro
    avrocpp
)

# Set Boost paths for Ubuntu
set(BOOST_ROOT "/usr")
set(BOOST_LIBRARYDIR "/usr/lib/x86_64-linux-gnu")
set(Boost_NO_SYSTEM_PATHS OFF)

# Find packages and print versions
find_package(spdlog REQUIRED)
message(STATUS "spdlog version: ${spdlog_VERSION}")

# Add pkg-config support
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_ZeroMQ QUIET libzmq)

# Set ZeroMQ paths explicitly
set(ZeroMQ_INCLUDE_DIRS "/usr/include")
set(ZeroMQ_LIBRARIES "zmq")
set(ZeroMQ_LIBRARY_DIRS "/usr/lib/x86_64-linux-gnu")
set(ZeroMQ_VERSION "4.3.4")  # This is the version in Ubuntu 22.04

message(STATUS "ZeroMQ include dirs: ${ZeroMQ_INCLUDE_DIRS}")
message(STATUS "ZeroMQ libraries: ${ZeroMQ_LIBRARIES}")
message(STATUS "ZeroMQ version: ${ZeroMQ_VERSION}")

find_package(Boost 1.73.0 REQUIRED COMPONENTS iostreams filesystem program_options regex)
message(STATUS "Boost version: ${Boost_VERSION}")

# Set paths for dependencies
set(JsonDom_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/avro/lang/c++/impl")  
set(Spdlog_INCLUDE_DIR "/usr/include")
set(Boost_INCLUDE_DIR "/usr/include")
set(Avro_LIBRARY "${AVRO_CPP_LIBRARY}")
set(Spdlog_LIBRARY "spdlog")

set(Boost_USE_STATIC_LIBS OFF)  
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# Add include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/include 
    ${Boost_INCLUDE_DIRS}
    ${Spdlog_INCLUDE_DIR} 
    ${INC_DIR} 
    "/include" 
    ${JsonDom_INCLUDE_DIR} 
    ${GS_COMM} 
    ${GS_SER}
    ${GS_SER}/ccsds/include  # Add CCSDS include directory
    ${ZeroMQ_INCLUDE_DIRS}  # Add ZeroMQ include dirs
    ${spdlog_INCLUDE_DIRS}  # Add spdlog include dirs
    ${AVRO_CPP_PATH}  # Add Avro include dir
)

# Add library paths
link_directories(
    ${AVRO_LIBRARY_DIR}
    ${Boost_LIBRARY_DIRS}
    ${ZeroMQ_LIBRARY_DIRS}  # Add ZeroMQ library dirs
    "/usr/lib/x86_64-linux-gnu"
)

# First, add the communication subdirectory to build dependencies
add_subdirectory(${GS_COMM})

set(RTADPPROTODIR "${CMAKE_SOURCE_DIR}/src/rtadp-proto/")

file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")

list(APPEND RTADPPROTO1FILES 
    ${RTADPPROTODIR}/ProcessDataConsumer1.cpp
    ${RTADPPROTODIR}/Supervisor1.cpp
    ${RTADPPROTODIR}/Worker1.cpp
    ${RTADPPROTODIR}/WorkerManager1.cpp
)

list(APPEND RTADPPROTO2FILES 
    ${RTADPPROTODIR}/ProcessDataConsumer2.cpp
    ${RTADPPROTODIR}/Supervisor2.cpp
    ${RTADPPROTODIR}/Worker2.cpp
    ${RTADPPROTODIR}/WorkerManager2.cpp
)

set(SOURCES ${RTADP_PROTO_FILES} ${SRC_FILES})

# Define the executables
add_executable(ProcessDataConsumer1 ${RTADPPROTO1FILES} ${SRC_FILES})
target_compile_options(ProcessDataConsumer1 PRIVATE -g)

add_executable(ProcessDataConsumer2 ${RTADPPROTO2FILES} ${SRC_FILES})
target_compile_options(ProcessDataConsumer2 PRIVATE -g)

########################
# cmake -DENABLE_LOGGING=ON ..
# cmake -DENABLE_LOGGING=OFF ..

if(ENABLE_LOGGING)  
    add_compile_definitions(ENABLE_LOGGING)
endif()
########################

# Add SPDLOG_ACTIVE_LEVEL definition
target_compile_definitions(ProcessDataConsumer1 PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
target_compile_definitions(ProcessDataConsumer2 PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)

# Link libraries using target_link_libraries for better dependency handling
target_link_libraries(ProcessDataConsumer1 PRIVATE
    zmq 
    ${LIBS}
    spdlog::spdlog
    ${Boost_LIBRARIES}
    pthread
    GSccsds
    avrocpp
)

target_link_libraries(ProcessDataConsumer2 PRIVATE
    zmq 
    ${LIBS}
    spdlog::spdlog
    ${Boost_LIBRARIES}
    pthread
    GSccsds
    avrocpp
)
