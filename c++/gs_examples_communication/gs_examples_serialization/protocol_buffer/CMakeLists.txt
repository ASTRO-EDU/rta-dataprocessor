# Minimum CMake required
cmake_minimum_required(VERSION 3.10)

# Project
project(gs_protobuf_serialization CXX)

# Ensure that we are using static libraries, including Protocol Buffers
set(protobuf_BUILD_SHARED_LIBS OFF)  # Disable shared libraries for protobuf
set(BUILD_SHARED_LIBS OFF)  # Globally disable shared libraries

# Find required protobuf package
find_package(protobuf CONFIG REQUIRED)

if(protobuf_VERBOSE)
  message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_build_my_MSVC_application_with_a_static_runtime.3F
if(MSVC AND protobuf_MSVC_STATIC_RUNTIME)
  foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
  endforeach()
endif()

set(Proto_Messages_SRC_DIR ${PROJECT_SOURCE_DIR}/messages_def)
set(TS_PROTOS "${Proto_Messages_SRC_DIR}/TimeSpec.proto")
set(WF_PROTOS "${Proto_Messages_SRC_DIR}/WF.proto")
set(HK_PROTOS "${Proto_Messages_SRC_DIR}/HK.proto")

#code generations

set(INC_DIR 
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/build"
  )

# Add the common source files
set(COMMON_SOURCES 
  "include/HKGenerator.cpp"
  "include/WFGenerator.cpp"
  "include/TimeSpecGen.cpp"
  "include/Utils.cpp"
)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

# Create a static library for GSProtoclBufferSerialization sources
add_library(GSProtoclBufferSerialization STATIC ${COMMON_SOURCES} ${TS_PROTOS} ${WF_PROTOS} ${HK_PROTOS})
target_include_directories(GSProtoclBufferSerialization PUBLIC ${INC_DIR} ${PROTO_BINARY_DIR})  # Ensure headers are included
target_link_libraries(GSProtoclBufferSerialization PUBLIC protobuf::libprotobuf)
protobuf_generate(TARGET GSProtoclBufferSerialization IMPORT_DIRS ${Proto_Messages_SRC_DIR} PROTOC_OUT_DIR ${PROTO_BINARY_DIR})

foreach(project "WF_main" "HK_main")

  set(${project}_SRCS ${SRC_DIR}/${project}.cpp)

  #Executable setup
  set(executable_name ${project}_cpp)
  add_executable(${executable_name} ${${project}_SRCS})
  target_include_directories(${executable_name} PUBLIC ${INC_DIR})
  target_link_libraries(${executable_name} GSProtoclBufferSerialization)
    

endforeach()
