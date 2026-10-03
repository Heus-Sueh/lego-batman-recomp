#include "win32_shims.h"
#include "win32_d3d9.h"
#include "../runtime/diagnostics.h"
#include "target_config.h"
#include <iostream>
#include <cstring>
#include <cstdlib>

#if !defined(_WIN32)
#include <unistd.h>
#include <pthread.h>
#endif

extern "C" ptrdiff_t g_mem_base;

namespace pcrecomp::win32 {

namespace {
thread_local uint32_t s_last_error = 0;
}

uint32_t shim_GetLastError() {
    return s_last_error;
}

void shim_SetLastError(uint32_t dwErrCode) {
    s_last_error = dwErrCode;
}

uint32_t shim_GetCurrentProcessId() {
#if defined(_WIN32)
    return GetCurrentProcessId();
#else
    return static_cast<uint32_t>(getpid());
#endif
}

uint32_t shim_GetCurrentThreadId() {
#if defined(_WIN32)
    return GetCurrentThreadId();
#else
    return static_cast<uint32_t>(pthread_self());
#endif
}

void shim_OutputDebugStringA(const char* lpOutputString) {
    if (lpOutputString) {
        std::cout << "[WIN32_DEBUG] " << lpOutputString;
    }
}

void shim_ExitProcess(uint32_t uExitCode) {
    std::cout << "[Win32] ExitProcess called with code " << uExitCode << "\n";
    std::exit(static_cast<int>(uExitCode));
}

void* shim_GetProcAddress(void* hModule, const char* lpProcName) {
    (void)hModule;
    (void)lpProcName;
    return nullptr;
}

static uint32_t g_tls_slots[64] = {0};
static uint32_t g_next_tls_idx = 0;

uint32_t shim_TlsAlloc() {
    if (g_next_tls_idx < 64) return g_next_tls_idx++;
    return 0xFFFFFFFFu;
}

int32_t shim_TlsSetValue(uint32_t dwTlsIndex, void* lpTlsValue) {
    if (dwTlsIndex < 64) {
        g_tls_slots[dwTlsIndex] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lpTlsValue));
        return 1;
    }
    return 0;
}

void* shim_TlsGetValue(uint32_t dwTlsIndex) {
    if (dwTlsIndex < 64) {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(g_tls_slots[dwTlsIndex]));
    }
    return nullptr;
}

int32_t shim_TlsFree(uint32_t dwTlsIndex) {
    if (dwTlsIndex < 64) {
        g_tls_slots[dwTlsIndex] = 0;
    }
    return 1;
}

int32_t shim_InterlockedIncrement(int32_t* lpAddend) {
    if (lpAddend) return ++(*lpAddend);
    return 0;
}

int32_t shim_InterlockedDecrement(int32_t* lpAddend) {
    if (lpAddend) return --(*lpAddend);
    return 0;
}

void* shim_GetModuleHandleA(const char* lpModuleName) {
    (void)lpModuleName;
    return reinterpret_cast<void*>(0x00400000);
}

const char* shim_GetCommandLineA(void) {
    return pcrecomp::target::guest_executable;
}

void shim_GetStartupInfoA(void* lpStartupInfo) {
    if (lpStartupInfo) {
        std::memset(lpStartupInfo, 0, 68);
        *reinterpret_cast<uint32_t*>(lpStartupInfo) = 68; // cb = sizeof(STARTUPINFOA)
    }
}

int32_t shim_GetVersionExA(void* lpVersionInformation) {
    if (!lpVersionInformation) return 0;
    auto* words = reinterpret_cast<uint32_t*>(lpVersionInformation);
    words[1] = 5;     // dwMajorVersion = 5 (Windows XP)
    words[2] = 1;     // dwMinorVersion = 1
    words[3] = 2600;  // dwBuildNumber = 2600
    words[4] = 2;     // dwPlatformId = VER_PLATFORM_WIN32_NT (2)
    char* szCSD = reinterpret_cast<char*>(lpVersionInformation) + 20;
    std::strncpy(szCSD, "Service Pack 3", 128);
    return 1;
}

uint32_t shim_SetErrorMode(uint32_t uMode) {
    (void)uMode;
    return 0;
}

void* shim_GetStdHandle(int32_t nStdHandle) {
    switch (nStdHandle) {
        case -10: return reinterpret_cast<void*>(0x00000010); // STD_INPUT_HANDLE
        case -11: return reinterpret_cast<void*>(0x00000014); // STD_OUTPUT_HANDLE
        case -12: return reinterpret_cast<void*>(0x00000018); // STD_ERROR_HANDLE
        default: return reinterpret_cast<void*>(0xFFFFFFFF);
    }
}

uint32_t shim_GetFileType(void* hFile) {
    (void)hFile;
    return 2; // FILE_TYPE_CHAR
}

uint32_t shim_SetHandleCount(uint32_t uNumber) {
    return uNumber;
}

void* shim_GetEnvironmentStringsW(void) {
    static const uint16_t s_env[] = {
        '=', ':', ':', '=', ':', ':', '\\', 0,
        'P', 'A', 'T', 'H', '=', '.', 0,
        'S', 'Y', 'S', 'T', 'E', 'M', 'R', 'O', 'O', 'T', '=', 'C', ':', '\\', 'W', 'i', 'n', 'd', 'o', 'w', 's', 0,
        0
    };
    uint32_t guest_va = 0x00030000;
    if (g_mem_base) {
        std::memcpy(reinterpret_cast<void*>(guest_va + g_mem_base), s_env, sizeof(s_env));
    }
    return reinterpret_cast<void*>(static_cast<uintptr_t>(guest_va));
}

int32_t shim_FreeEnvironmentStringsW(void* penv) {
    (void)penv;
    return 1;
}

int32_t shim_WideCharToMultiByte(uint32_t CodePage, uint32_t dwFlags, const uint16_t* lpWideCharStr,
                                 int32_t cchWideChar, char* lpMultiByteStr, int32_t cbMultiByte,
                                 const char* lpDefaultChar, int32_t* lpUsedDefaultChar) {
    (void)CodePage;
    (void)dwFlags;
    (void)lpDefaultChar;
    (void)lpUsedDefaultChar;
    if (cchWideChar < 0) {
        int count = 0;
        if (lpWideCharStr) {
            while (lpWideCharStr[count] != 0) count++;
            count++;
        }
        cchWideChar = count;
    }
    if (cbMultiByte == 0) {
        return cchWideChar;
    }
    int to_copy = (cchWideChar < cbMultiByte) ? cchWideChar : cbMultiByte;
    if (lpWideCharStr && lpMultiByteStr) {
        for (int i = 0; i < to_copy; ++i) {
            lpMultiByteStr[i] = static_cast<char>(lpWideCharStr[i] & 0x7F);
        }
    }
    return to_copy;
}

uint32_t shim_GetACP(void) {
    return 0; // CP_ACP
}

uint32_t shim_GetOEMCP(void) {
    return 0;
}

int32_t shim_IsValidCodePage(uint32_t CodePage) {
    (void)CodePage;
    return 1;
}

int32_t shim_GetCPInfo(uint32_t CodePage, void* lpCPInfo) {
    (void)CodePage;
    if (lpCPInfo) {
        std::memset(lpCPInfo, 0, 18);
        *reinterpret_cast<uint32_t*>(lpCPInfo) = 1; // MaxCharSize = 1
    }
    return 1;
}

void* shim_SetUnhandledExceptionFilter(void* lpTopLevelExceptionFilter) {
    (void)lpTopLevelExceptionFilter;
    return nullptr;
}

void initialize_win32_subsystems() {
    std::cout << "[Win32] Compatibility layer initialized (File I/O, Threads, Windowing, Clocks).\n";
    init_d3d9_subsystem();
}

} // namespace pcrecomp::win32
