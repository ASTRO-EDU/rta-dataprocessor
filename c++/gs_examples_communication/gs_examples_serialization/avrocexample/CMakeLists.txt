cmake_minimum_required(VERSION 3.10)
project(AvroSerialize C)

set(CMAKE_C_STANDARD 99)

# Specify the paths to the Avro include directory and library
set(AVRO_INCLUDE_DIR /include)
set(AVRO_LIBRARY_DIR /include/lib)
set(AVRO_LIBRARIES avro)

# Include the Avro headers
include_directories(${AVRO_INCLUDE_DIR})

# Link the Avro library
link_directories(${AVRO_LIBRARY_DIR})

# Add the executable
add_executable(AvroSerialize src/serialExample.c)

# Link the Avro library to the executable
target_link_libraries(AvroSerialize ${AVRO_LIBRARIES})
