cmake_minimum_required(VERSION 3.10)
project(RTADataProcessor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Installation directories
set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Install prefix")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;/usr/lib")

# Required dependencies for the framework core
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_ZeroMQ REQUIRED libzmq)

# Avro libraries
set(AVRO_DIR /usr/local/lib CACHE PATH "Avro library directory")
set(AVRO_INCLUDE_DIR /usr/local/include CACHE PATH "Avro include directory")
set(AVRO_VERSION 1.11.0)

pkg_check_modules(AVRO avro-cpp>=${AVRO_VERSION})
if(NOT AVRO_FOUND)
    message(STATUS "avro-cpp pkg-config module not found, using direct library paths")
    set(AVRO_LIBRARY_DIRS ${AVRO_DIR})
    set(AVRO_INCLUDE_DIRS ${AVRO_INCLUDE_DIR})
endif()

find_library(AVROCPP_LIBRARY 
    NAMES avrocpp libavrocpp.so
    PATHS ${AVRO_LIBRARY_DIRS} ${AVRO_DIR}
    REQUIRED
)

# Framework source files
file(GLOB FRAMEWORK_SOURCES "src/*.cpp")

# Create the framework library
add_library(rtadp-framework SHARED ${FRAMEWORK_SOURCES})

# Create alias to match the installed target name (for add_subdirectory usage)
add_library(RTADP::rtadp-framework ALIAS rtadp-framework)

# Public include directories
target_include_directories(rtadp-framework PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${AVRO_INCLUDE_DIRS}
    ${PC_ZeroMQ_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(rtadp-framework PUBLIC
    ${PC_ZeroMQ_LIBRARIES}
    ${AVROCPP_LIBRARY}
    pthread
)

# Compile options
target_compile_options(rtadp-framework PRIVATE -Wall -Wextra -g)
target_compile_definitions(rtadp-framework PUBLIC 
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO
    SPDLOG_HEADER_ONLY
)

# Install rules
install(TARGETS rtadp-framework
    EXPORT RTADataProcessorTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/rtadp/
    DESTINATION include/rtadp
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hh" PATTERN "*.hpp"
)

# Export configuration
install(EXPORT RTADataProcessorTargets
    FILE RTADataProcessorTargets.cmake
    NAMESPACE RTADP::
    DESTINATION lib/cmake/RTADataProcessor
)

# Create and install package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RTADataProcessorConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/RTADataProcessorConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/RTADataProcessorConfig.cmake"
    INSTALL_DESTINATION lib/cmake/RTADataProcessor
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RTADataProcessorConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RTADataProcessorConfigVersion.cmake"
    DESTINATION lib/cmake/RTADataProcessor
)

# Print configuration summary
message(STATUS "=== RTA Data Processor Framework ===")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "ZeroMQ: ${PC_ZeroMQ_LIBRARIES}")
message(STATUS "Avro: ${AVROCPP_LIBRARY}")
message(STATUS "====================================")

# CPack configuration for tarball package generation
set(CPACK_PACKAGE_NAME "rta-dataprocessor-framework")
set(CPACK_PACKAGE_VENDOR "ASTRO-EDU")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RTA Data Processor Framework - High-performance data processing framework")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Generate tarball only
set(CPACK_GENERATOR "TGZ")

include(CPack)
