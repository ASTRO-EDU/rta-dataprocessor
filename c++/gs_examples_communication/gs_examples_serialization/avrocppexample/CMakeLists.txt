cmake_minimum_required(VERSION 3.10)
project(AvroSerializer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC")

# Specify the paths to the Avro include directory and library
set(AVRO_CPP_PATH "/usr/local/include/avro")

set(INC_DIR
    "include"  # Include directory within the small project
    "${AVRO_CPP_PATH}"             # Avro include directory
)

find_library(AVRO_CPP_LIBRARY avrocpp_s REQUIRED PATHS ${AVRO_CPP_PATH}/../../lib)

list(APPEND LIBS 
    avrocpp_s
)

link_directories(${AVRO_LIBRARY_DIR})

# Add the common source files
set(COMMON_SOURCES 
    include/HKGenerator.cpp
    include/WFGenerator.cpp
    include/Utils.cpp
)

# Add the source files for HKtest
set(HKTEST_SOURCES 
    src/HKtest.cpp
    ${COMMON_SOURCES}
)

# Add the source files for WFtest
set(WFTEST_SOURCES 
    src/WFtest.cpp
    ${COMMON_SOURCES}
)

# Create a static library for HKtest sources
add_library(GSAvroSerialization SHARED ${COMMON_SOURCES})
target_include_directories(GSAvroSerialization PUBLIC ${INC_DIR})  # Ensure headers are included
target_link_libraries(GSAvroSerialization PUBLIC ${LIBS})

# Optionally, you can still build the test executables using the libraries
add_executable(HkSerializerTest ${HKTEST_SOURCES})
target_include_directories(HkSerializerTest PUBLIC ${INC_DIR})
target_link_libraries(HkSerializerTest LINK_PUBLIC GSAvroSerialization)

add_executable(WfSerializerTest ${WFTEST_SOURCES})
target_include_directories(WfSerializerTest PUBLIC ${INC_DIR})
target_link_libraries(WfSerializerTest LINK_PUBLIC GSAvroSerialization)
