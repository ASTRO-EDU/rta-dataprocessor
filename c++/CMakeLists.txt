cmake_minimum_required(VERSION 3.10)

project(DataProcessor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

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

# Aggiungi sottodirectory
add_subdirectory(${GS_COMM})

# Crea gli eseguibili
add_executable(ProcessDataConsumer1 ${RTADPPROTO1FILES} ${SRC_FILES})
add_executable(ProcessDataConsumer2 ${RTADPPROTO2FILES} ${SRC_FILES})

# Opzioni di compilazione
target_compile_options(ProcessDataConsumer1 PRIVATE -g)
target_compile_options(ProcessDataConsumer2 PRIVATE -g)

if(ENABLE_LOGGING)
    add_compile_definitions(ENABLE_LOGGING)
endif()

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