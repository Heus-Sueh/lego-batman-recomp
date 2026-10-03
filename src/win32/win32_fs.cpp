#include "win32_shims.h"
#include "../runtime/diagnostics.h"
#include <win32compat/fs.h>
#include <pcmodern/filesystem.h>
#include <iostream>
#include <cstring>
#include <filesystem>

namespace pcrecomp::win32 {

void* shim_CreateFileA(const char* lpFileName, uint32_t dwDesiredAccess, uint32_t dwShareMode,
                      void* lpSecurityAttributes, uint32_t dwCreationDisposition,
                      uint32_t dwFlagsAndAttributes, void* hTemplateFile) {
    uint32_t handle_id = win32compat::create_file_a(
        lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

    if (runtime::get_trace_config().trace_files) {
        if (handle_id == win32compat::WIN32_INVALID_HANDLE_VALUE) {
            std::cout << "[TRACE:FILES] CreateFileA FAILED: \"" << (lpFileName ? lpFileName : "<null>") << "\"\n";
        } else {
            std::cout << "[TRACE:FILES] CreateFileA: \"" << (lpFileName ? lpFileName : "") << "\" -> handle 0x"
                      << std::hex << handle_id << std::dec << "\n";
        }
    }

    shim_SetLastError(win32compat::get_last_error());
    if (handle_id == win32compat::WIN32_INVALID_HANDLE_VALUE) {
        return reinterpret_cast<void*>(-1);
    }
    return reinterpret_cast<void*>(static_cast<uintptr_t>(handle_id));
}

int32_t shim_ReadFile(void* hFile, void* lpBuffer, uint32_t nNumberOfBytesToRead,
                     uint32_t* lpNumberOfBytesRead, void* lpOverlapped) {
    if (!hFile || hFile == reinterpret_cast<void*>(-1)) {
        return 0;
    }
    uint32_t handle_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hFile));
    int32_t result = win32compat::read_file(handle_id, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    shim_SetLastError(win32compat::get_last_error());

    if (runtime::get_trace_config().trace_files) {
        uint32_t read_bytes = lpNumberOfBytesRead ? *lpNumberOfBytesRead : 0;
        std::cout << "[TRACE:FILES] ReadFile handle 0x" << std::hex << handle_id << std::dec
                  << " read " << read_bytes << "/" << nNumberOfBytesToRead << " bytes\n";
    }

    return result;
}

int32_t shim_WriteFile(void* hFile, const void* lpBuffer, uint32_t nNumberOfBytesToWrite,
                      uint32_t* lpNumberOfBytesWritten, void* lpOverlapped) {
    if (!hFile || hFile == reinterpret_cast<void*>(-1)) {
        return 0;
    }
    uint32_t handle_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hFile));
    int32_t result = win32compat::write_file(handle_id, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    shim_SetLastError(win32compat::get_last_error());

    if (runtime::get_trace_config().trace_files) {
        uint32_t written = lpNumberOfBytesWritten ? *lpNumberOfBytesWritten : 0;
        std::cout << "[TRACE:FILES] WriteFile handle 0x" << std::hex << handle_id << std::dec
                  << " wrote " << written << " bytes\n";
    }

    return result;
}

int32_t shim_CloseHandle(void* hObject) {
    if (!hObject || hObject == reinterpret_cast<void*>(-1)) {
        return 0;
    }
    uint32_t handle_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hObject));
    if (runtime::get_trace_config().trace_files) {
        std::cout << "[TRACE:FILES] CloseHandle 0x" << std::hex << handle_id << std::dec << "\n";
    }
    int32_t result = win32compat::close_handle(handle_id);
    shim_SetLastError(win32compat::get_last_error());
    return result;
}

uint32_t shim_SetFilePointer(void* hFile, int32_t lDistanceToMove, int32_t* lpDistanceToMoveHigh, uint32_t dwMoveMethod) {
    if (!hFile || hFile == reinterpret_cast<void*>(-1)) {
        return static_cast<uint32_t>(-1);
    }
    uint32_t handle_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hFile));
    uint32_t result = win32compat::set_file_pointer(handle_id, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
    shim_SetLastError(win32compat::get_last_error());
    return result;
}

uint32_t shim_GetFileSize(void* hFile, uint32_t* lpFileSizeHigh) {
    if (!hFile || hFile == reinterpret_cast<void*>(-1)) {
        return static_cast<uint32_t>(-1);
    }
    uint32_t handle_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(hFile));
    uint32_t result = win32compat::get_file_size(handle_id, lpFileSizeHigh);
    shim_SetLastError(win32compat::get_last_error());
    return result;
}

uint32_t shim_GetFileAttributesA(const char* lpFileName) {
    if (!lpFileName) return static_cast<uint32_t>(-1);

    auto resolved = pcmodern::VirtualFileSystem::instance().resolve(lpFileName);
    if (!resolved) {
        if (std::filesystem::exists(lpFileName)) {
            resolved = std::filesystem::path(lpFileName);
        } else {
            return static_cast<uint32_t>(-1);
        }
    }

    if (std::filesystem::is_directory(*resolved)) {
        return 0x10; // FILE_ATTRIBUTE_DIRECTORY
    }
    return 0x80; // FILE_ATTRIBUTE_NORMAL
}

uint32_t shim_fopen_handle(const char* filename, const char* mode) {
    if (!filename || !mode) return 0;
    uint32_t desired_access = win32compat::WIN32_GENERIC_READ;
    uint32_t disposition = win32compat::WIN32_OPEN_EXISTING;
    if (std::strchr(mode, 'w') || std::strchr(mode, '+')) {
        desired_access |= win32compat::WIN32_GENERIC_WRITE;
        disposition = win32compat::WIN32_CREATE_ALWAYS;
    }
    uint32_t handle = win32compat::create_file_a(filename, desired_access, 0, nullptr, disposition, 0, nullptr);
    return (handle == win32compat::WIN32_INVALID_HANDLE_VALUE) ? 0 : handle;
}

} // namespace pcrecomp::win32
