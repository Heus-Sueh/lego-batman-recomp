# Shared legacy bridge dependencies remain until codegen removes unused RW bodies.
include(${PROJECT_SOURCE_DIR}/src/adapters/common/bridge_dependencies.cmake)
list(APPEND RECOMP_ADAPTER_SOURCES src/adapters/lego_batman/application.cpp)

set(_target_manifest "${RECOMP_TARGET_CONFIG}")
set(RECOMP_TARGET_CONFIG "${PROJECT_SOURCE_DIR}/${RECOMP_TARGET_ACTIVE_REVISION}/revision.toml")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${RECOMP_TARGET_CONFIG}")
recomp_target_value(entry_point_va LEGO_ENTRY)
recomp_target_value(image_base LEGO_IMAGE_BASE)
recomp_target_value(stack_top LEGO_STACK_TOP)
recomp_target_integer(file_size LEGO_FILE_SIZE)
set(RECOMP_TARGET_CONFIG "${_target_manifest}")
configure_file(${CMAKE_CURRENT_LIST_DIR}/revision.h.in
    ${PROJECT_BINARY_DIR}/generated/lego_revision.h @ONLY)
