# ------ Optimization flags for Jetson Nano inference ------
set(OPT_FLAGS
  -O2
  -mcpu=cortex-a78
  -ftree-vectorize
  -funsafe-math-optimizations
)
# Add these flags globally
add_compile_options(${OPT_FLAGS})

cmake_minimum_required(VERSION 3.10)

project(DataProcessor C CXX)

# Force use of system compiler
# set(CMAKE_C_COMPILER "/usr/bin/gcc")
# set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
<<<<<<< HEAD

set(CMAKE_INSTALL_PREFIX "/usr")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;/usr/lib")

# ----------------------------
# TensorFlow Lite C API Setup
# ----------------------------
# Directory where you built TFLite C API (libtensorflowlite_c.so)
set(TFLITE_C_BUILD_DIR "$ENV{HOME}/tflite_build" CACHE PATH
    "Path to your TFLite C API build directory")

# Locate the TFLite C library
find_library(TFLITE_C_LIB
    NAMES tensorflowlite_c
    PATHS "${TFLITE_C_BUILD_DIR}/lib" "${TFLITE_C_BUILD_DIR}"
    NO_DEFAULT_PATH
)
if(NOT TFLITE_C_LIB)
    message(FATAL_ERROR "Could not find libtensorflowlite_c.so in ${TFLITE_C_BUILD_DIR}")
endif()

# Include the TFLite C API headers
include_directories(
    "${TFLITE_C_BUILD_DIR}/include"   # installed headers from build
    "$ENV{HOME}/tensorflow_src"        # source root so 'tensorflow/lite/c/c_api.h' resolves
)
# ----------------------------
=======
set(CMAKE_BUILD_TYPE Debug)
>>>>>>> c++_ab

# Per abilitare logging con: cmake -DENABLE_LOGGING=ON ..
option(ENABLE_LOGGING "Enable logging" OFF)

# Trova pacchetti
find_package(Boost 1.73 REQUIRED COMPONENTS iostreams filesystem program_options regex)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_ZeroMQ REQUIRED libzmq)

# Avro libraries (avrocpp is the common name)
# Try to use pkg-config first, but fallback if not available
set(AVRO_DIR /usr/local/lib)
set(AVRO_INCLUDE_DIR /usr/local/include)
set(AVRO_VERSION 1.11.0)

pkg_check_modules(AVRO avro-cpp>=${AVRO_VERSION})
if(NOT AVRO_FOUND)
    message(STATUS "avro-cpp pkg-config module not found, using direct library paths")
    set(AVRO_LIBRARY_DIRS ${AVRO_DIR})
    set(AVRO_INCLUDE_DIRS ${AVRO_INCLUDE_DIR})
endif()

find_library(AVRO_CPP_LIBRARY 
    NAMES 
        avro
        libavro.so.${AVRO_VERSION}
    PATHS
        ${AVRO_LIBRARY_DIRS}
        ${AVRO_DIR}
    REQUIRED)
find_library(AVROCPP_LIBRARY 
    NAMES 
        avrocpp
        libavrocpp.so.${AVRO_VERSION}
    PATHS
        ${AVRO_LIBRARY_DIRS}
        ${AVRO_DIR}
    REQUIRED)

include_directories(${AVRO_INCLUDE_DIRS})
link_directories(${AVRO_LIBRARY_DIRS})

# Directory locali
set(GS_COMM ${PROJECT_SOURCE_DIR}/gs_examples_communication)
set(GS_SER ${GS_COMM}/gs_examples_serialization)
<<<<<<< HEAD
set(BUILD_CCSDS ON)

# Set paths for Avro
set(AVRO_CPP_PATH "/usr/local/include/avro")

set(INC_DIR
    "include"
    "${AVRO_CPP_PATH}"
)

find_library(AVRO_CPP_LIBRARY avrocpp_s REQUIRED PATHS ${AVRO_CPP_PATH}/../../lib)

list(APPEND LIBS 
    avrocpp_s
)

# Set Boost paths for Ubuntu
set(BOOST_ROOT "/usr")
set(BOOST_LIBRARYDIR "/usr/lib/x86_64-linux-gnu")
set(Boost_NO_SYSTEM_PATHS OFF)

# Find packages
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_ZeroMQ QUIET libzmq)

# Explicit ZeroMQ paths
set(ZeroMQ_INCLUDE_DIRS "/usr/include")
set(ZeroMQ_LIBRARIES "zmq")
set(ZeroMQ_LIBRARY_DIRS "/usr/lib/x86_64-linux-gnu")
set(ZeroMQ_VERSION "4.3.4")

message(STATUS "ZeroMQ include dirs: ${ZeroMQ_INCLUDE_DIRS}")
message(STATUS "ZeroMQ libraries: ${ZeroMQ_LIBRARIES}")

find_package(Boost 1.73.0 REQUIRED COMPONENTS iostreams filesystem program_options regex)

message(STATUS "spdlog version: ${spdlog_VERSION}")
message(STATUS "ZeroMQ version: ${ZeroMQ_VERSION}")
message(STATUS "Boost version: ${Boost_VERSION}")

# Other include paths
set(JsonDom_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/avro/lang/c++/impl")
set(Spdlog_INCLUDE_DIR "/usr/include")
set(Boost_INCLUDE_DIR "/usr/include")

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${Spdlog_INCLUDE_DIR}
    ${INC_DIR}
    "/include"
    ${JsonDom_INCLUDE_DIR}
    ${GS_COMM}
    ${GS_SER}
    ${GS_SER}/ccsds/include
    ${ZeroMQ_INCLUDE_DIRS}
    ${spdlog_INCLUDE_DIRS}
    ${AVRO_CPP_PATH}
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${ZeroMQ_LIBRARY_DIRS}
    "/usr/lib/x86_64-linux-gnu"
)

# Build communication library first
add_subdirectory(${GS_COMM})

# Source & proto paths
set(RTADPPROTODIR "${CMAKE_SOURCE_DIR}/src/rtadp-proto/")
=======
set(RTADPPROTODIR "${CMAKE_SOURCE_DIR}/src/rtadp-proto/")

# Includi le directory necessarie
include_directories(
    ${CMAKE_SOURCE_DIR}/include 
    ${GS_COMM} 
    ${GS_SER}
    ${GS_SER}/ccsds/include
    ${Boost_INCLUDE_DIRS}
    ${PC_ZeroMQ_INCLUDE_DIRS}
)

# Collega le librerie
link_directories(
    ${Boost_LIBRARY_DIRS}
    ${PC_ZeroMQ_LIBRARY_DIRS}
)

# File sorgente
>>>>>>> c++_ab
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

<<<<<<< HEAD
# Executables
=======
# Aggiungi sottodirectory
add_subdirectory(${GS_COMM})

# Crea gli eseguibili
>>>>>>> c++_ab
add_executable(ProcessDataConsumer1 ${RTADPPROTO1FILES} ${SRC_FILES})
add_executable(ProcessDataConsumer2 ${RTADPPROTO2FILES} ${SRC_FILES})

# Opzioni di compilazione
target_compile_options(ProcessDataConsumer1 PRIVATE -g)
target_compile_options(ProcessDataConsumer2 PRIVATE -g)

<<<<<<< HEAD
# Optional logging flag
=======
>>>>>>> c++_ab
if(ENABLE_LOGGING)
    add_compile_definitions(ENABLE_LOGGING)
endif()

<<<<<<< HEAD
# spdlog level
target_compile_definitions(ProcessDataConsumer1 PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
target_compile_definitions(ProcessDataConsumer2 PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)

# Link libraries
target_link_libraries(ProcessDataConsumer1 PRIVATE
    zmq
    ${LIBS}
    spdlog::spdlog
    ${Boost_LIBRARIES}
    pthread
    GSccsds
    avrocpp
    ${TFLITE_C_LIB}  # Link TFLite C API
)

target_link_libraries(ProcessDataConsumer2 PRIVATE
    zmq
    ${LIBS}
    spdlog::spdlog
    ${Boost_LIBRARIES}
    pthread
    GSccsds
    avrocpp
    ${TFLITE_C_LIB}  # Link TFLite C API
)
=======
# Log level
target_compile_definitions(ProcessDataConsumer1 PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
target_compile_definitions(ProcessDataConsumer2 PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)

# Link alle librerie
target_link_libraries(ProcessDataConsumer1 PRIVATE
    ${AVRO_CPP_LIBRARY}
    ${AVROCPP_LIBRARY}
    ${PC_ZeroMQ_LIBRARIES}
    ${Boost_LIBRARIES}
    pthread
    GSccsds
    spdlog::spdlog
)

target_link_libraries(ProcessDataConsumer2 PRIVATE
    ${AVRO_CPP_LIBRARY}
    ${AVROCPP_LIBRARY}
    ${PC_ZeroMQ_LIBRARIES}
    ${Boost_LIBRARIES}
    pthread
    GSccsds
    spdlog::spdlog
)
>>>>>>> c++_ab
