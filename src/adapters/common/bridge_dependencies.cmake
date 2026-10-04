# Core project platform and runtime dependencies
find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
    add_library(SDL3::SDL3 ALIAS PkgConfig::SDL3)
endif()

# Core project sources
set(RECOMP_ADAPTER_SOURCES
    src/runtime/bridge_adapter.cpp
    src/runtime/guest_memory.cpp
    src/runtime/guest_heap.cpp
    src/runtime/section_loader.cpp
    src/runtime/dispatch.cpp
    src/runtime/diagnostics.cpp
    src/platform/platform_sdl.cpp
    src/win32/win32_timing.cpp
    src/win32/win32_threads.cpp
    src/win32/win32_memory.cpp
    src/win32/win32_fs.cpp
    src/win32/win32_process.cpp
    src/win32/win32_windowing.cpp
    src/win32/win32_audio.cpp
    src/win32/win32_d3d9.cpp
    src/win32/import_resolver.cpp
    src/renderer/renderer.cpp
)


set(RECOMP_ADAPTER_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/thirdparty/dxvk-native/include/dxvk"
    "${PROJECT_SOURCE_DIR}/thirdparty/dxvk-native/include")
set(RECOMP_ADAPTER_LIBRARIES
    pcrecomp::win32_time pcrecomp::win32_fs SDL3::SDL3
    "${PROJECT_SOURCE_DIR}/thirdparty/dxvk-native/lib/libdxvk_d3d9.so")
if(UNIX AND NOT APPLE)
    list(APPEND RECOMP_ADAPTER_LIBRARIES pthread dl)
    set(RECOMP_ADAPTER_BUILD_RPATH "${PROJECT_SOURCE_DIR}/thirdparty/dxvk-native/lib")
    set(RECOMP_ADAPTER_INSTALL_RPATH "$ORIGIN/../../../thirdparty/dxvk-native/lib")
endif()
