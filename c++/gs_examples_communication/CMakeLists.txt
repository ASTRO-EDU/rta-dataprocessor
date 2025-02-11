cmake_minimum_required(VERSION 3.10)
# set(CMAKE_SYSTEM_PROCESSOR x86_64)

# set(CMAKE_C_COMPILER x86_64-linux-gnu-gcc)
# set(CMAKE_CXX_COMPILER "x86_64-linux-gnu-g++")
project(GSCommunication CXX)

# Ensure that we are using static libraries, including Protocol Buffers

set(BUILD_SHARED_LIBS OFF)  # Globally disable shared libraries

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
# Find required protobuf package
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(ZMQ_DIR ${PROJECT_SOURCE_DIR}/src/zeromq)
set(SHM_DIR ${PROJECT_SOURCE_DIR}/src/shared_memory)
set(GS_SERIALIZATION_DIR ${PROJECT_SOURCE_DIR}/gs_examples_serialization)

# Define custom options to enable/disable building of specific targets
option(BUILD_EXAMPLES "Build basic ZMQ examples" OFF)
option(BUILD_AVRO "Build Avro-based examples" OFF)
option(BUILD_PB "Build Protocol Buffer examples" OFF)
option(BUILD_CCSDS "Build CCSDS examples" ON)
option(BUILD_SHM "Build Shared Memory example" OFF)

# Add Avro subdirectory conditionally
if(BUILD_AVRO)
    add_subdirectory(${GS_SERIALIZATION_DIR}/avrocppexample)
endif()

# Add Protocol Buffer subdirectory conditionally
if(BUILD_PB)
    set(protobuf_BUILD_SHARED_LIBS OFF)  # Disable shared libraries for protobuf
    find_package(protobuf CONFIG REQUIRED)
    add_subdirectory(${GS_SERIALIZATION_DIR}/protocol_buffer)
endif()

# Add CCSDS subdirectory conditionally
if(BUILD_CCSDS)
    add_subdirectory(${GS_SERIALIZATION_DIR}/ccsds)
endif()

# Common include and source files
set(INC_DIR
    ${PROJECT_SOURCE_DIR}/include
)

set(COMMON_SRCS
    ${PROJECT_SOURCE_DIR}/include/utils.cc
)

set(ZMQ_INC_DIR
    ${INC_DIR}
    ${ZMQ_DIR}/include
)

set(ZMQ_COMMON_SRCS
    ${COMMON_SRCS}
)

find_package(cppzmq REQUIRED)
message(STATUS "cppzmq version: ${cppzmq_VERSION}")


list(APPEND LIBS 
    cppzmq
)

# Conditionally add executables based on the selected options
if(BUILD_EXAMPLES)
    add_executable(prodEx ${ZMQ_DIR}/example/prodEx.cpp ${ZMQ_COMMON_SRCS})
    add_executable(consEx ${ZMQ_DIR}/example/consEx.cpp ${ZMQ_COMMON_SRCS})

    target_include_directories(prodEx PUBLIC ${ZMQ_INC_DIR})
    target_include_directories(consEx PUBLIC ${ZMQ_INC_DIR})
    target_link_libraries(prodEx PUBLIC ${LIBS})
    target_link_libraries(consEx PUBLIC ${LIBS})
endif()

if(BUILD_AVRO)
    set(AVRO_LIBS
        ${LIBS}
        GSAvroSerialization
    )

    add_executable(AvroCppProd ${ZMQ_DIR}/avrocpp/AvroProducer.cpp ${ZMQ_COMMON_SRCS})
    add_executable(AvroCppCons ${ZMQ_DIR}/avrocpp/AvroConsumer.cpp ${ZMQ_COMMON_SRCS})

    target_include_directories(AvroCppProd PUBLIC ${ZMQ_INC_DIR})
    target_include_directories(AvroCppCons PUBLIC ${ZMQ_INC_DIR})
    target_link_libraries(AvroCppProd LINK_PUBLIC ${AVRO_LIBS})
    target_link_libraries(AvroCppCons LINK_PUBLIC ${AVRO_LIBS})
endif()

if(BUILD_PB)
    set(GSProtoclBufferSerialization_DIR ${GS_SERIALIZATION_DIR}/protocol_buffer/build)

    find_library(GSProtoclBufferSerialization_LIB
                 NAMES GSProtoclBufferSerialization
                 PATHS ${GSProtoclBufferSerialization_DIR})

    set(PB_DIRS
      ${ZMQ_INC_DIR}
      ${GS_SERIALIZATION_DIR}/protocol_buffer
      ${GS_SERIALIZATION_DIR}/protocol_buffer/include
    )
    set(PB_LIBS
      ${LIBS}
      protobuf::libprotobuf
      GSProtoclBufferSerialization
    )
    add_executable(PBCppProd ${ZMQ_DIR}/protobuf/ProtoProducer.cpp ${ZMQ_COMMON_SRCS})
    add_executable(PBCppCons ${ZMQ_DIR}/protobuf/ProtoConsumer.cpp ${ZMQ_COMMON_SRCS})

    target_include_directories(PBCppProd PUBLIC ${PB_DIRS})
    target_include_directories(PBCppCons PUBLIC ${PB_DIRS})
    target_link_libraries(PBCppProd PUBLIC ${PB_LIBS})
    target_link_libraries(PBCppCons LINK_PUBLIC ${PB_LIBS})

    add_dependencies(PBCppProd GSProtoclBufferSerialization)
    add_dependencies(PBCppCons GSProtoclBufferSerialization)
endif()

if(BUILD_CCSDS)
    set(CCSDS_DIRS
      ${ZMQ_INC_DIR}
      ${GS_SERIALIZATION_DIR}/ccsds/include
    )
    set(CCSDS_LIBS
      ${LIBS}
      GSccsds
    )
    add_executable(CCSDSCppProd ${ZMQ_DIR}/ccsds/CCSDSProducer.cpp ${ZMQ_COMMON_SRCS})
    add_executable(CCSDSCppCons ${ZMQ_DIR}/ccsds/CCSDSConsumer.cpp ${ZMQ_COMMON_SRCS})

    target_include_directories(CCSDSCppProd PUBLIC ${CCSDS_DIRS})
    target_include_directories(CCSDSCppCons PUBLIC ${ZMQ_INC_DIR})
    target_link_libraries(CCSDSCppProd PUBLIC ${CCSDS_LIBS})
    target_link_libraries(CCSDSCppCons PUBLIC ${CCSDS_LIBS})
endif()

# Configuration File:
set(SOURCE_FILE ${ZMQ_DIR}/config.txt)
set(DESTINATION_FILE ${CMAKE_BINARY_DIR}/config.txt)
file(COPY ${SOURCE_FILE} DESTINATION ${CMAKE_BINARY_DIR})

if(BUILD_SHM)
    set(SOURCE_DIR
    ${SHM_DIR}/ProdCons.cpp
    )

    # Add the executable
    add_executable(shmEx ${SOURCE_DIR})
endif()
