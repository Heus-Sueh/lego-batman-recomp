#pragma once
#include <cstdint>
#include <cstddef>

namespace win32compat {

inline constexpr uint32_t WIN32_GENERIC_READ          = 0x80000000u;
inline constexpr uint32_t WIN32_GENERIC_WRITE         = 0x40000000u;
inline constexpr uint32_t WIN32_CREATE_NEW            = 1u;
inline constexpr uint32_t WIN32_CREATE_ALWAYS         = 2u;
inline constexpr uint32_t WIN32_OPEN_EXISTING         = 3u;
inline constexpr uint32_t WIN32_OPEN_ALWAYS           = 4u;
inline constexpr uint32_t WIN32_TRUNCATE_EXISTING     = 5u;
inline constexpr uint32_t WIN32_INVALID_HANDLE_VALUE  = 0xFFFFFFFFu;
inline constexpr uint32_t WIN32_INVALID_SET_FILE_POINTER = 0xFFFFFFFFu;
inline constexpr uint32_t WIN32_INVALID_FILE_SIZE     = 0xFFFFFFFFu;

inline constexpr uint32_t WIN32_ERROR_SUCCESS         = 0u;
inline constexpr uint32_t WIN32_ERROR_FILE_NOT_FOUND  = 2u;
inline constexpr uint32_t WIN32_ERROR_ACCESS_DENIED   = 5u;
inline constexpr uint32_t WIN32_ERROR_INVALID_HANDLE  = 6u;
inline constexpr uint32_t WIN32_ERROR_INVALID_PARAMETER = 87u;

uint32_t get_last_error();
void set_last_error(uint32_t error);

uint32_t create_file_a(const char* file_name, uint32_t desired_access, uint32_t share_mode,
                       void* security_attributes, uint32_t creation_disposition,
                       uint32_t flags_and_attributes, void* template_file);

int32_t read_file(uint32_t handle, void* buffer, uint32_t bytes_to_read,
                  uint32_t* bytes_read, void* overlapped);

int32_t write_file(uint32_t handle, const void* buffer, uint32_t bytes_to_write,
                   uint32_t* bytes_written, void* overlapped);

uint32_t set_file_pointer(uint32_t handle, int32_t distance_to_move,
                          int32_t* distance_to_move_high, uint32_t move_method);

uint32_t get_file_size(uint32_t handle, uint32_t* file_size_high);

int32_t close_handle(uint32_t handle);

} // namespace win32compat
