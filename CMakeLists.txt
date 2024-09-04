cmake_minimum_required(VERSION 3.23)
project(
    Godot-Kafka
    VERSION 0.1.0
    LANGUAGES CXX
)

# Generate the project files
# execute_process(
# COMMAND scons
# WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
# )

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set CMake Predefined Targets Folders
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "External/CMake")

# Output the binaries to the bin folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/demo/client/bin/${CMAKE_SYSTEM_NAME}>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/demo/server/bin/${CMAKE_SYSTEM_NAME}>")

# Prefix all the binaries with "lib"
set(CMAKE_SHARED_LIBRARY_PREFIX "lib")

# Glob all sources files.
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp" "src/*.h")

add_library(GodotKafka SHARED
    ${SOURCES}
    ${HEADERS}
)

# Include Godot::cpp
add_subdirectory(godot-cpp)
target_link_libraries(GodotKafka PRIVATE godot::cpp)
set_target_properties(godot-cpp PROPERTIES FOLDER "External/Godot")

# Include librdkafka
set(RDKAFKA_BUILD_EXAMPLES OFF)
set(RDKAFKA_BUILD_TESTS OFF)
add_subdirectory(extern/librdkafka)
target_link_libraries(GodotKafka PRIVATE rdkafka) # rdkafka is the C library and the core.
target_link_libraries(GodotKafka PRIVATE rdkafka++) # rdkafka++ is a C++ wrapper around librdkafka
set_target_properties(rdkafka PROPERTIES FOLDER "External/rdkafka")
set_target_properties(rdkafka++ PROPERTIES FOLDER "External/rdkafka")

find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
    message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_libraries(OpenSSL::Crypto) 
endif()