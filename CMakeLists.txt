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

target_link_libraries( ${PROJECT_NAME} PRIVATE godot-cpp::editor )

get_target_property( GODOT_PLATFORM godot-cpp::editor GODOT_PLATFORM )
get_target_property( GODOT_TARGET godot-cpp::editor GODOT_TARGET )
get_target_property( GODOT_ARCH godot-cpp::editor GODOT_ARCH )
#get_target_property( GODOT_DEV godot-cpp::editor GODOT_DEV )
#get_target_property( GODOT_DOUBLE godot-cpp::editor GODOT_DOUBLE )

#set( DEV ".${GODOT_DEV}" )
#set( DOUBLE ".${GODOT_DOUBLE}" )

set_target_properties( ${PROJECT_NAME}
        PROPERTIES
        #The generator expression here prevents a subdir from being created.
        RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}>"
        # name format project.<platform>.<target>[.dev][.double].<arch>[.custom_suffix]
        OUTPUT_NAME "${PROJECT_NAME}.${GODOT_PLATFORM}.${GODOT_TARGET}${DEV}${DOUBLE}.${GODOT_ARCH}"
)
