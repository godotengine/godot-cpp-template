
function(target_autogoc TARGET ROOT_DIR)
    get_target_property(SOURCE_DIR ${TARGET} SOURCE_DIR)
    get_target_property(BINARY_DIR ${TARGET} BINARY_DIR)
    get_target_property(SOURCES ${TARGET} SOURCES)
    get_target_property(INCLUDE_DIRECTORIES ${TARGET} INCLUDE_DIRECTORIES)

    set(GOC_GENERATED_DIR ${BINARY_DIR}/.goc/generated)
    set(GOC_GENERATED_FILES "")
    list(APPEND GOC_GENERATED_FILES ${GOC_GENERATED_DIR}/generated_register_types.h)
    list(APPEND GOC_GENERATED_FILES ${GOC_GENERATED_DIR}/generated_register_types.cpp)
    foreach (SOURCE_PATH ${SOURCES})
        cmake_path(RELATIVE_PATH SOURCE_PATH BASE_DIRECTORY ${ROOT_DIR} OUTPUT_VARIABLE RELATIVE)
        cmake_path(REPLACE_EXTENSION RELATIVE "generated.h" OUTPUT_VARIABLE GENERATED_H)
        cmake_path(REPLACE_EXTENSION RELATIVE "generated.cpp" OUTPUT_VARIABLE GENERATED_CPP)
        list(APPEND GOC_GENERATED_FILES ${GOC_GENERATED_DIR}/${GENERATED_H})
        list(APPEND GOC_GENERATED_FILES ${GOC_GENERATED_DIR}/${GENERATED_CPP})
    endforeach ()

    if (TARGET godot-cpp)
        get_target_property(GODOT_CPP_FOLDER godot-cpp SOURCE_DIR)
        set(GDEXTENSION_API_FILE ${GODOT_CPP_FOLDER}/gdextension/extension_api.json)
        get_target_property(GODOT_CPP_INCLUDE_DIRECTORIES godot-cpp INCLUDE_DIRECTORIES)
    else ()
        message(FATAL_ERROR "AUTOGOC: target godot-cpp not found.")
    endif ()

    list(JOIN INCLUDE_DIRECTORIES "," INCLUDE_JOINED)
    list(JOIN SOURCES "," SOURCES_JOINED)
    list(JOIN GODOT_CPP_INCLUDE_DIRECTORIES "," GODOT_CPP_INCLUDE_DIRECTORIES_JOINED)

    if (TARGET goc)
        get_target_property(GOC_BINARY_DIR goc BINARY_DIR)
        message(VERBOSE "GOC: Using goc target executable ${GOC_BINARY_DIR}/goc")

        add_custom_target(RunGOC
                SOURCES ${SOURCES}
                BYPRODUCTS ${GOC_GENERATED_FILES}
                COMMAND ${GOC_BINARY_DIR}/goc${CMAKE_EXECUTABLE_SUFFIX} generate
                -R=${ROOT_DIR}
                -P=.goc
                -C=.goc/cache
                -G=.goc/generated
                -I=${INCLUDE_JOINED}
                -S=${SOURCES_JOINED}
                -GPP=${GODOT_CPP_INCLUDE_DIRECTORIES_JOINED}
                -E=${GDEXTENSION_API_FILE}
                WORKING_DIRECTORY ${BINARY_DIR}
                DEPENDS goc godot-cpp generate_bindings ${SOURCES}
                COMMENT "GOC: Generating Bindings"
        )
    elseif (DEFINED ENV{GOC_EXECUTABLE})
        message(VERBOSE "GOC: Using goc executable ${GOC_EXECUTABLE}")
        add_custom_target(RunGOC
                SOURCES ${SOURCES}
                BYPRODUCTS ${GOC_GENERATED_FILES}
                COMMAND $ENV{GOC_EXECUTABLE} generate
                -R=${ROOT_DIR}
                -P=.goc
                -C=.goc/cache
                -G=.goc/generated
                -I=${INCLUDE_JOINED}
                -S=${SOURCES_JOINED}
                -GPP=${GODOT_CPP_INCLUDE_DIRECTORIES_JOINED}
                -E=${GDEXTENSION_API_FILE}
                WORKING_DIRECTORY ${BINARY_DIR}
                DEPENDS godot-cpp generate_bindings ${SOURCES}
                COMMENT "GOC: Generating Bindings"
        )
    else ()
        message(FATAL_ERROR "AUTOGOC: goc executable not found\n"
                "Please set GOC_EXECUTABLE environment variable to the path of the goc executable\n"
                "If you have the goc repository cloned, you can alternatively add the target subdirectory using add_subdirectory(<path_to_goc_repository>).\n"
        )
    endif ()

    target_include_directories(${TARGET} PRIVATE
            ${GOC_GENERATED_DIR}
    )

    add_dependencies(${TARGET} RunGOC)
    target_sources(${TARGET}
            PRIVATE
            ${GOC_GENERATED_FILES}
    )
endfunction()
