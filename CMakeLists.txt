cmake_minimum_required(VERSION 3.28)

# The name of the project is used for the build target
project( godot-cpp-template
        VERSION 1.0
        DESCRIPTION ""
        LANGUAGES CXX)

if( CMAKE_C_COMPILER )
    #Silence unused variable when specified from toolchain
endif ()

add_subdirectory( godot-cpp )

add_library( ${PROJECT_NAME} SHARED )

target_sources( ${PROJECT_NAME}
        PRIVATE
        src/register_types.cpp
        src/register_types.h
)

target_link_libraries( ${PROJECT_NAME} PRIVATE godot-cpp )

set( ARCH_NAME_AMD64 x86_64 )

set( PLATFORM_NAME "$<LOWER_CASE:$<PLATFORM_ID>>")
set( TARGET_NAME "$<LOWER_CASE:$<CONFIG>>")
set( DOUBLE "$<$<STREQUAL:${FLOAT_PRECISION},double>:.double>")
set( ARCH_NAME ${ARCH_NAME_${CMAKE_SYSTEM_PROCESSOR}})

set_target_properties( ${PROJECT_NAME}
        PROPERTIES
        #The generator expression here prevents a subdir from being created.
        RUNTIME_OUTPUT_DIRECTORY "$<$<BOOL:1>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}>"
        # godot.<platform>.<target>[.dev][.double].<arch>[.custom_suffix][.console].exe
        OUTPUT_NAME "${PROJECT_NAME}.${PLATFORM_NAME}.${TARGET_NAME}${DOUBLE}.${ARCH_NAME}"
)
