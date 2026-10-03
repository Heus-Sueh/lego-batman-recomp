cmake_minimum_required(VERSION 3.25)

set(RECOMP_TARGET_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/config/target.toml" CACHE FILEPATH
    "Target manifest used to configure the recompilation")
if(NOT EXISTS "${RECOMP_TARGET_CONFIG}")
    message(FATAL_ERROR "Target manifest not found: ${RECOMP_TARGET_CONFIG}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${RECOMP_TARGET_CONFIG}")
function(recomp_target_value key out_var)
    file(STRINGS "${RECOMP_TARGET_CONFIG}" value_line REGEX "^[ \t]*${key}[ \t]*=[ \t]*\"[^\"]*\"[ \t]*$")
    if(NOT value_line)
        message(FATAL_ERROR "Missing string '${key}' in ${RECOMP_TARGET_CONFIG}")
    endif()
    list(GET value_line 0 value_line)
    string(REGEX REPLACE "^[ \t]*${key}[ \t]*=[ \t]*\"([^\"]*)\"[ \t]*$" "\\1" value "${value_line}")
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()
function(recomp_target_integer key out_var)
    file(STRINGS "${RECOMP_TARGET_CONFIG}" value_line REGEX "^[ \t]*${key}[ \t]*=[ \t]*[0-9]+[ \t]*$")
    if(NOT value_line)
        message(FATAL_ERROR "Missing integer '${key}' in ${RECOMP_TARGET_CONFIG}")
    endif()
    list(GET value_line 0 value_line)
    string(REGEX REPLACE "^[^=]*=[ \t]*([0-9]+)[ \t]*$" "\\1" value "${value_line}")
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()
recomp_target_value(id RECOMP_TARGET_ID)
recomp_target_value(title RECOMP_TARGET_TITLE)
recomp_target_value(output_name RECOMP_TARGET_OUTPUT_NAME)
recomp_target_value(guest_executable RECOMP_TARGET_GUEST_EXECUTABLE)
recomp_target_value(default_game_dir RECOMP_TARGET_DEFAULT_GAME_DIR)
recomp_target_value(active_revision RECOMP_TARGET_ACTIVE_REVISION)
recomp_target_integer(simulation_fps RECOMP_TARGET_SIMULATION_FPS)
recomp_target_integer(default_width RECOMP_TARGET_DEFAULT_WIDTH)
recomp_target_integer(default_height RECOMP_TARGET_DEFAULT_HEIGHT)
configure_file(src/runtime/target_config.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/target_config.h" @ONLY)

project(${RECOMP_TARGET_ID}_recomp
    VERSION 0.1.0
    DESCRIPTION "Native static recompilation target"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and options
add_compile_definitions(RECOMP_TRACE)
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
endif()

# Find SDL3
add_subdirectory(pcmodern)
add_subdirectory(libpcrecomp)
add_subdirectory(win32compat)
add_library(pcrecomp_win32_time STATIC libpcrecomp/src/win32_time.cpp)
add_library(pcrecomp::win32_time ALIAS pcrecomp_win32_time)
target_link_libraries(pcrecomp_win32_time PUBLIC pcrecomp::bridge win32compat::kernel32)

add_library(pcrecomp_win32_fs STATIC libpcrecomp/src/win32_fs.cpp)
add_library(pcrecomp::win32_fs ALIAS pcrecomp_win32_fs)
target_link_libraries(pcrecomp_win32_fs PUBLIC pcrecomp::bridge win32compat::kernel32)

option(PCRECOMP_BUILD_GAME "Build the configured game integration (requires SDL and generated code)" ON)
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests/runtime)
endif()
if(NOT PCRECOMP_BUILD_GAME)
    return()
endif()

recomp_target_value(adapter RECOMP_TARGET_ADAPTER)
if(NOT RECOMP_TARGET_ADAPTER MATCHES "^(legacy_pc|lego_batman)$")
    message(FATAL_ERROR "Unsupported target adapter: ${RECOMP_TARGET_ADAPTER}")
endif()
include(src/adapters/${RECOMP_TARGET_ADAPTER}/adapter.cmake)
set(RECOMP_SOURCES src/main.cpp ${RECOMP_ADAPTER_SOURCES})

# Optional generated recomp sources
file(GLOB_RECURSE GENERATED_RECOMP_SOURCES CONFIGURE_DEPENDS "generated/recomp/*.cpp")
if(GENERATED_RECOMP_SOURCES)
    list(APPEND RECOMP_SOURCES ${GENERATED_RECOMP_SOURCES})
    message(STATUS "Including ${GENERATED_RECOMP_SOURCES} generated recomp files")
endif()

# Main executable
add_executable(${RECOMP_TARGET_OUTPUT_NAME} ${RECOMP_SOURCES})
target_compile_definitions(${RECOMP_TARGET_OUTPUT_NAME} PRIVATE RECOMP_RUNTIME_DIAGNOSTICS)
if(RECOMP_TARGET_ADAPTER STREQUAL "legacy_pc")
    target_compile_definitions(${RECOMP_TARGET_OUTPUT_NAME} PRIVATE RECOMP_LEGACY_GAME_HOOKS)
endif()

target_include_directories(${RECOMP_TARGET_OUTPUT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/recomp
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_include_directories(${RECOMP_TARGET_OUTPUT_NAME} SYSTEM PRIVATE
    ${RECOMP_ADAPTER_INCLUDE_DIRS}
)

target_link_libraries(${RECOMP_TARGET_OUTPUT_NAME} PRIVATE
    ${RECOMP_ADAPTER_LIBRARIES}
)

if(UNIX AND NOT APPLE)
    set_target_properties(${RECOMP_TARGET_OUTPUT_NAME} PROPERTIES
        BUILD_RPATH "${RECOMP_ADAPTER_BUILD_RPATH}"
        INSTALL_RPATH "${RECOMP_ADAPTER_INSTALL_RPATH}"
    )
endif()

message(STATUS "Configured recomp target: ${RECOMP_TARGET_ID} (${RECOMP_TARGET_OUTPUT_NAME})")
