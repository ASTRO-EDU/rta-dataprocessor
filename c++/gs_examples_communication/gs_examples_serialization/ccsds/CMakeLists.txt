# Remove project definition since we're in a subdirectory
# project(AvroSerialize CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

set(INC_DIR
	"${PROJECT_SOURCE_DIR}/include"
	)

set(COMMON_SOURCES 
    include/packet.cpp  # Add all relevant .cpp files
    include/generators.cpp
    # Add other .cpp files if necessary
)

# Create the library with proper visibility
add_library(GSccsds STATIC ${COMMON_SOURCES})
target_include_directories(GSccsds PUBLIC ${INC_DIR})

# Add the executables
add_executable(HKtest src/HKtest.cpp)
add_executable(WFtest src/WFtest.cpp)

target_include_directories(HKtest PUBLIC ${INC_DIR})
target_include_directories(WFtest PUBLIC ${INC_DIR})

target_link_libraries(HKtest PUBLIC GSccsds)
target_link_libraries(WFtest PUBLIC GSccsds)
# Link the Avro library to the executable
