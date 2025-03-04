# Using the same minimum as the godot-cpp project
cmake_minimum_required(VERSION 3.17)

#[[ Things to know

The default build type in CMake is 'Debug' which optimises for and generates debugging symbols.
To compile a roughly equivalent build as a default SCons build, further action is required:
    * For Ninja, or Makefile; add '-DCMAKE_BUILD_TYPE=Release' to the configure command
    * For Ninja-Multi, Visual Studio, or XCode; add '--config Release' to the build command
]]

set(LIBNAME "EXTENSION-NAME" CACHE STRING "The name of the library")

set(GODOT_PROJECT_DIR "demo" CACHE STRING "The directory of a Godot project folder")

set(GODOT_TARGET_TYPE "template_debug" CACHE STRING "Which target type to build against")
set_property(CACHE GODOT_TARGET_TYPE PROPERTY STRINGS "template_debug;template_release;editor")

# Specify Options
option(USE_GIT_SUBMODULES "Use the git submodules to fetch godot-cpp" ON)
option(USE_FETCHCONTENT "Use CMake's FetchContent to fetch godot-cpp" OFF)

# Verify Options
if(USE_GIT_SUBMODULES AND USE_FETCHCONTENT)
    message(FATAL_ERROR "USE_GIT_SUBMODULES and USE_FETCHCONTENT are mutually exclusive options")
endif()

#[[ CMake has a bunch of global properties which get copied to projects and targets at the moment of definition.
So make any changes to global options which effect all components of the build prior to making any call that
leads to a project or target definition.
Examples are: CMAKE_OSX_ARCHITECTURES, CMAKE_MSVC_RUNTIME_LIBRARY
]]

# Silence unused variable warning when specified from toolchain
if(CMAKE_C_COMPILER)
endif()

# Make sure all the dependencies are satisfied
find_package(Python3 3.4 REQUIRED)

# For godot-cpp we can use the git submodule method, or we can use CMake's fetchcontent module
# In either case it is important to specify any GODOTCPP_ options prior to add_subdirectory
# or fetchcontent_makeavailable

# I highly recommend using a build profile to cut down on the code generation and build time of godot-cpp
# set(GODOTCPP_BUILD_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/build_profile.json")

if(USE_GIT_SUBMODULES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/src")
        message(NOTICE "godot-cpp bindings source not found")
        message(NOTICE "initializing/updating the godot-cpp submodule...")

        # update the c++ bindings submodule to populate it with the necessary source for the library
        execute_process(
            COMMAND git submodule update --init godot-cpp
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()
    add_subdirectory(godot-cpp)
endif()

if(USE_FETCHCONTENT)
    include(FetchContent)
    # Godot-cpp
    set(GODOTCPP_GIT_URL "http://github.com/godotengine/godot-cpp.git" CACHE STRING "The git url of godot-cpp to fetch")
    set(GODOTCPP_GIT_BRANCH "master" CACHE STRING "The git branch of godot-cpp to fetch")

    fetchcontent_declare(godot-cpp
        GIT_REPOSITORY ${GODOTCPP_GIT_URL}
        GIT_TAG ${GODOTCPP_GIT_BRANCH}
        GIT_PROGRESS ON
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp"
    )
    fetchcontent_makeavailable(godot-cpp)
endif()

# Add godot-cpp's module path and include the exported functions.
# This is made available for documentation generation
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/cmake")
include(GodotCPPModule)

# The godot-cpp cmake project specifies three targets to link against.
# godot-cpp.template-debug
# godot-cpp.template_release
# godot-cpp.editor

# Helper variable to prevent typo's
set(GODOTCPP_TARGET godot-cpp.${GODOT_TARGET_TYPE})

# The targets have some of useful properties attached to them that can be retrieved like so.
get_target_property(GODOTCPP_SUFFIX ${GODOTCPP_TARGET} GODOTCPP_SUFFIX)
get_target_property(GODOTCPP_PLATFORM ${GODOTCPP_TARGET} GODOTCPP_PLATFORM)

# Now we can specify our own project which will inherit any global cmake properties or variables that have been defined.
project(godot-cpp-template
    VERSION 1.0
    DESCRIPTION "This repository serves as a quickstart template for GDExtension development with Godot 4.0+."
    HOMEPAGE_URL "https://github.com/enetheru/godot-cpp-template/tree/main"
    LANGUAGES CXX
)

add_library(${LIBNAME} SHARED)

target_sources(${LIBNAME}
    PRIVATE
    src/register_types.cpp
    src/register_types.h
)

# Fetch a list of the xml files to use for documentation and add to our target
file(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/doc_classes/*.xml")

# conditionally add doc data to compile output
if(DOC_XML)
    if(GODOT_TARGET_TYPE MATCHES "editor|template_debug")
        target_doc_sources(${LIBNAME} ${DOC_XML})
    endif()
endif()

target_link_libraries(${LIBNAME} PRIVATE ${GODOTCPP_TARGET})

set_target_properties(${LIBNAME}
    PROPERTIES
    # The generator expression here prevents msvc from adding a Debug or Release subdir.
    RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_SOURCE_DIR}/${GODOT_PROJECT_DIR}/bin/${GODOTCPP_PLATFORM}>"

    PREFIX lib
    OUTPUT_NAME "${LIBNAME}${GODOTCPP_SUFFIX}"
)
