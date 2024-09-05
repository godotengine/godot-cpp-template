cmake_minimum_required(VERSION 3.23)
project(
    Godot-Kafka
    VERSION 0.1.0
    LANGUAGES CXX
)

option(BUILD_TESTS "Build the tests" ON)

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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/demo/client/addons/gdkafka/bin/${CMAKE_SYSTEM_NAME}>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/demo/server/addons/gdkafka/bin/${CMAKE_SYSTEM_NAME}>")

# Prefix all the binaries with "lib"
set(CMAKE_SHARED_LIBRARY_PREFIX "lib")

# Glob all sources files.
file(GLOB_RECURSE GODOT_SOURCES "src/godotkafka/*.cpp")
file(GLOB_RECURSE GODOT_HEADERS "src/godotkafka/*.hpp" "src/godotkafka/*.h")
file(GLOB_RECURSE TEST_SOURCES "src/test/*.cpp")
file(GLOB_RECURSE TEST_HEADERS "src/test/*.hpp" "src/test/*.h")
file(GLOB_RECURSE LIB_SOURCES "src/kafkalib/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "src/kafkalib/*.hpp" "src/kafkalib/*.h" "include/kafkalib/*.hpp" "include/kafkalib/*.h")

# Include OpenSSL
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
set(system_library_extension "lib")
if (UNIX)
    set(system_library_extension "a")
endif()
find_library(CRYPTO_LIB libcrypto.${system_library_extension} REQUIRED)
find_library(SSL_LIB libssl.${system_library_extension} REQUIRED)

if(OPENSSL_FOUND)
    message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_libraries(OpenSSL::Crypto)
endif()

if (CRYPTO_LIB)
    message(STATUS "Found libcrypto: ${CRYPTO_LIB}")
    link_libraries(${CRYPTO_LIB})
endif()

if (SSL_LIB)
    message(STATUS "Found libssl: ${SSL_LIB}")
    link_libraries(${SSL_LIB})
endif()

add_library(KafkaLib STATIC 
    ${LIB_SOURCES}
    ${LIB_HEADERS}
)
target_include_directories(KafkaLib PUBLIC include/kafkalib)

add_library(GodotKafka SHARED
    ${GODOT_SOURCES}
    ${GODOT_HEADERS}
)

# Include Godot::cpp
add_subdirectory(godot-cpp)
target_link_libraries(GodotKafka PRIVATE godot::cpp)
target_link_libraries(GodotKafka PRIVATE KafkaLib)
set_target_properties(godot-cpp PROPERTIES FOLDER "External/Godot")

# Include librdkafka
set(RDKAFKA_BUILD_EXAMPLES OFF)
set(RDKAFKA_BUILD_TESTS OFF)
set(RDKAFKA_BUILD_STATIC ON)
add_subdirectory(extern/librdkafka)
target_link_libraries(KafkaLib PUBLIC rdkafka) # rdkafka is the C library and the core.
target_link_libraries(KafkaLib PUBLIC rdkafka++) # rdkafka++ is a C++ wrapper around librdkafka
set_target_properties(rdkafka PROPERTIES FOLDER "External/rdkafka")
set_target_properties(rdkafka++ PROPERTIES FOLDER "External/rdkafka")

if (BUILD_TESTS)
    add_executable(TestKafkaLib 
        ${TEST_SOURCES}
        ${TEST_HEADERS}
    )

    target_link_libraries(TestKafkaLib PRIVATE KafkaLib)

    # Force Visual Studio to set this target as the startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TestKafkaLib)
endif()