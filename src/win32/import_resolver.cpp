#include "import_resolver.h"
#include "win32_shims.h"
#include "win32_d3d9.h"
#include "../runtime/diagnostics.h"
#include <iostream>
#include <algorithm>

namespace pcrecomp::win32 {

ImportResolver& ImportResolver::instance() {
    static ImportResolver s_instance;
    return s_instance;
}

static std::string make_key(std::string module_name, std::string func_name) {
    std::transform(module_name.begin(), module_name.end(), module_name.begin(), ::tolower);
    return module_name + "!" + func_name;
}

void ImportResolver::register_shim(const std::string& module_name, const std::string& function_name, void* fn_ptr) {
    m_shims[make_key(module_name, function_name)] = fn_ptr;
}

void* ImportResolver::resolve_import(const std::string& module_name, const std::string& function_name) const {
    auto it = m_shims.find(make_key(module_name, function_name));
    if (it != m_shims.end()) {
        if (runtime::get_trace_config().trace_imports) {
            std::cout << "[TRACE:IMPORTS] Resolved " << module_name << "!" << function_name << "\n";
        }
        return it->second;
    }

    std::cerr << "[ImportResolver] Warning: Unmapped import: " << module_name << "!" << function_name << "\n";
    return nullptr;
}

size_t ImportResolver::registered_count() const noexcept {
    return m_shims.size();
}

void ImportResolver::initialize() {
    // 1. KERNEL32.dll
    register_shim("KERNEL32.dll", "timeGetTime", reinterpret_cast<void*>(shim_timeGetTime));
    register_shim("KERNEL32.dll", "GetTickCount", reinterpret_cast<void*>(shim_GetTickCount));
    register_shim("KERNEL32.dll", "QueryPerformanceCounter", reinterpret_cast<void*>(shim_QueryPerformanceCounter));
    register_shim("KERNEL32.dll", "QueryPerformanceFrequency", reinterpret_cast<void*>(shim_QueryPerformanceFrequency));
    register_shim("KERNEL32.dll", "CreateMutexA", reinterpret_cast<void*>(shim_CreateMutexA));
    register_shim("KERNEL32.dll", "ReleaseMutex", reinterpret_cast<void*>(shim_ReleaseMutex));
    register_shim("KERNEL32.dll", "GetCurrentProcessId", reinterpret_cast<void*>(shim_GetCurrentProcessId));
    register_shim("KERNEL32.dll", "GetCurrentThreadId", reinterpret_cast<void*>(shim_GetCurrentThreadId));
    register_shim("KERNEL32.dll", "OutputDebugStringA", reinterpret_cast<void*>(shim_OutputDebugStringA));
    register_shim("KERNEL32.dll", "GetLastError", reinterpret_cast<void*>(shim_GetLastError));
    register_shim("KERNEL32.dll", "SetLastError", reinterpret_cast<void*>(shim_SetLastError));
    register_shim("KERNEL32.dll", "ExitProcess", reinterpret_cast<void*>(shim_ExitProcess));
    register_shim("KERNEL32.dll", "VirtualAlloc", reinterpret_cast<void*>(shim_VirtualAlloc));
    register_shim("KERNEL32.dll", "VirtualProtect", reinterpret_cast<void*>(shim_VirtualProtect));
    register_shim("KERNEL32.dll", "VirtualFree", reinterpret_cast<void*>(shim_VirtualFree));
    register_shim("KERNEL32.dll", "HeapCreate", reinterpret_cast<void*>(shim_HeapCreate));
    register_shim("KERNEL32.dll", "HeapDestroy", reinterpret_cast<void*>(shim_HeapDestroy));
    register_shim("KERNEL32.dll", "HeapSize", reinterpret_cast<void*>(shim_HeapSize));
    register_shim("KERNEL32.dll", "GetProcAddress", reinterpret_cast<void*>(shim_GetProcAddress));
    register_shim("KERNEL32.dll", "TlsAlloc", reinterpret_cast<void*>(shim_TlsAlloc));
    register_shim("KERNEL32.dll", "TlsSetValue", reinterpret_cast<void*>(shim_TlsSetValue));
    register_shim("KERNEL32.dll", "TlsGetValue", reinterpret_cast<void*>(shim_TlsGetValue));
    register_shim("KERNEL32.dll", "TlsFree", reinterpret_cast<void*>(shim_TlsFree));
    register_shim("KERNEL32.dll", "InterlockedIncrement", reinterpret_cast<void*>(shim_InterlockedIncrement));
    register_shim("KERNEL32.dll", "InterlockedDecrement", reinterpret_cast<void*>(shim_InterlockedDecrement));
    register_shim("KERNEL32.dll", "CreateFileA", reinterpret_cast<void*>(shim_CreateFileA));
    register_shim("KERNEL32.dll", "ReadFile", reinterpret_cast<void*>(shim_ReadFile));
    register_shim("KERNEL32.dll", "WriteFile", reinterpret_cast<void*>(shim_WriteFile));
    register_shim("KERNEL32.dll", "CloseHandle", reinterpret_cast<void*>(shim_CloseHandle));
    register_shim("KERNEL32.dll", "SetFilePointer", reinterpret_cast<void*>(shim_SetFilePointer));
    register_shim("KERNEL32.dll", "GetFileSize", reinterpret_cast<void*>(shim_GetFileSize));
    register_shim("KERNEL32.dll", "GetFileAttributesA", reinterpret_cast<void*>(shim_GetFileAttributesA));
    register_shim("KERNEL32.dll", "GetModuleHandleA", reinterpret_cast<void*>(shim_GetModuleHandleA));
    register_shim("KERNEL32.dll", "GetCommandLineA", reinterpret_cast<void*>(shim_GetCommandLineA));
    register_shim("KERNEL32.dll", "GetStartupInfoA", reinterpret_cast<void*>(shim_GetStartupInfoA));
    register_shim("KERNEL32.dll", "GetVersionExA", reinterpret_cast<void*>(shim_GetVersionExA));
    register_shim("KERNEL32.dll", "CreateEventA", reinterpret_cast<void*>(shim_CreateEventA));
    register_shim("KERNEL32.dll", "Sleep", reinterpret_cast<void*>(shim_Sleep));
    register_shim("KERNEL32.dll", "SetErrorMode", reinterpret_cast<void*>(shim_SetErrorMode));
    register_shim("KERNEL32.dll", "GetStdHandle", reinterpret_cast<void*>(shim_GetStdHandle));
    register_shim("KERNEL32.dll", "GetFileType", reinterpret_cast<void*>(shim_GetFileType));
    register_shim("KERNEL32.dll", "SetHandleCount", reinterpret_cast<void*>(shim_SetHandleCount));
    register_shim("KERNEL32.dll", "GetEnvironmentStringsW", reinterpret_cast<void*>(shim_GetEnvironmentStringsW));
    register_shim("KERNEL32.dll", "FreeEnvironmentStringsW", reinterpret_cast<void*>(shim_FreeEnvironmentStringsW));
    register_shim("KERNEL32.dll", "WideCharToMultiByte", reinterpret_cast<void*>(shim_WideCharToMultiByte));
    register_shim("KERNEL32.dll", "GetACP", reinterpret_cast<void*>(shim_GetACP));
    register_shim("KERNEL32.dll", "GetOEMCP", reinterpret_cast<void*>(shim_GetOEMCP));
    register_shim("KERNEL32.dll", "IsValidCodePage", reinterpret_cast<void*>(shim_IsValidCodePage));
    register_shim("KERNEL32.dll", "GetCPInfo", reinterpret_cast<void*>(shim_GetCPInfo));
    register_shim("KERNEL32.dll", "SetUnhandledExceptionFilter", reinterpret_cast<void*>(shim_SetUnhandledExceptionFilter));

    // 2. USER32.dll
    register_shim("USER32.dll", "CreateWindowExA", reinterpret_cast<void*>(shim_CreateWindowExA));
    register_shim("USER32.dll", "ShowWindow", reinterpret_cast<void*>(shim_ShowWindow));
    register_shim("USER32.dll", "DestroyWindow", reinterpret_cast<void*>(shim_DestroyWindow));
    register_shim("USER32.dll", "AdjustWindowRectEx", reinterpret_cast<void*>(shim_AdjustWindowRectEx));
    register_shim("USER32.dll", "AdjustWindowRect", reinterpret_cast<void*>(shim_AdjustWindowRect));
    register_shim("USER32.dll", "SetWindowTextA", reinterpret_cast<void*>(shim_SetWindowTextA));
    register_shim("USER32.dll", "PeekMessageA", reinterpret_cast<void*>(shim_PeekMessageA));
    register_shim("USER32.dll", "DefWindowProcA", reinterpret_cast<void*>(shim_DefWindowProcA));
    register_shim("USER32.dll", "ShowCursor", reinterpret_cast<void*>(shim_ShowCursor));
    register_shim("USER32.dll", "PostQuitMessage", reinterpret_cast<void*>(shim_PostQuitMessage));
    register_shim("USER32.dll", "LoadIconA", reinterpret_cast<void*>(shim_LoadIconA));
    register_shim("USER32.dll", "LoadCursorA", reinterpret_cast<void*>(shim_LoadCursorA));
    register_shim("USER32.dll", "RegisterClassA", reinterpret_cast<void*>(shim_RegisterClassA));
    register_shim("USER32.dll", "FindWindowA", reinterpret_cast<void*>(shim_FindWindowA));
    register_shim("USER32.dll", "SetForegroundWindow", reinterpret_cast<void*>(shim_SetForegroundWindow));
    register_shim("USER32.dll", "GetWindowPlacement", reinterpret_cast<void*>(shim_GetWindowPlacement));
    register_shim("USER32.dll", "SystemParametersInfoA", reinterpret_cast<void*>(shim_SystemParametersInfoA));
    register_shim("USER32.dll", "SetWindowLongA", reinterpret_cast<void*>(shim_SetWindowLongA));
    register_shim("USER32.dll", "UpdateWindow", reinterpret_cast<void*>(shim_UpdateWindow));
    register_shim("USER32.dll", "SetWindowPos", reinterpret_cast<void*>(shim_SetWindowPos));

    // 3. WINMM.dll
    register_shim("WINMM.dll", "timeGetTime", reinterpret_cast<void*>(shim_timeGetTime));

    // 4. vorbisfile.dll
    register_shim("vorbisfile.dll", "ov_open_callbacks", reinterpret_cast<void*>(shim_ov_open_callbacks));
    register_shim("vorbisfile.dll", "ov_clear", reinterpret_cast<void*>(shim_ov_clear));
    register_shim("vorbisfile.dll", "ov_read", reinterpret_cast<void*>(shim_ov_read));

    // 5. d3d9.dll
    register_shim("d3d9.dll", "Direct3DCreate9", reinterpret_cast<void*>(shim_Direct3DCreate9));

    std::cout << "[ImportResolver] Initialized " << m_shims.size() << " API shims.\n";
}

void initialize_win32_imports() {
    ImportResolver::instance().initialize();
}

} // namespace pcrecomp::win32
