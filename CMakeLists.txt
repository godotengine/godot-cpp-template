# Using the same minimum as the godot-cpp project
cmake_minimum_required(VERSION 3.17)

# Specify Options
option(USE_GIT_SUBMODULES ON "")
option(USE_FETCHCONTENT OFF "")

# Verify Options
if(USE_GIT_SUBMODULES AND USE_FETCHCONTENT)
    message(FATAL_ERROR "Cannot specify both git submodules and fetchcontent.")
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

#set(GODOTCPP_BUILD_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/build_profile.json")

if(USE_GIT_SUBMODULES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/godot-cpp/src")
        message(NOTICE "godot-cpp bindings source not found")
        message(NOTICE "initializing/updating the godot-cpp submodule...")

        # update the c++ bindings submodule to populate it with
        # the necessary source for the library
        execute_process(
            COMMAND git submodule update --init extern/godot-cpp
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

# Now we can specify our own project.
project(godot-cpp-template
    VERSION 1.0
    DESCRIPTION "This repository serves as a quickstart template for GDExtension development with Godot 4.0+."
    HOMEPAGE_URL "https://github.com/enetheru/godot-cpp-template/tree/main"
    LANGUAGES CXX
)

# The PROJECT_NAME stores the name of the last called project()
add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
    PRIVATE
    src/register_types.cpp
    src/register_types.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE godot-cpp.editor)

# The godot targets have a couple of useful properties attached to them
get_target_property(GODOTCPP_SUFFIX godot-cpp.editor GODOTCPP_SUFFIX)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    #The generator expression here prevents a subdir from being created.
    RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}>"
    # name format project.<platform>.<target>[.dev][.double].<arch>[.custom_suffix]
    OUTPUT_NAME "${PROJECT_NAME}${GODOTCPP_SUFFIX}"
)
