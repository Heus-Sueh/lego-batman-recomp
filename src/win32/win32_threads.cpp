#include "win32_shims.h"
#include "../platform/platform.h"
#include "../runtime/diagnostics.h"
#include <iostream>
#include <win32compat/time.h>

namespace pcrecomp::win32 {

void* shim_CreateMutexA(void* lpMutexAttributes, int32_t bInitialOwner, const char* lpName) {
    (void)lpMutexAttributes;
    (void)bInitialOwner;
    if (runtime::get_trace_config().trace_threads) {
        std::cout << "[TRACE:THREADS] CreateMutexA: \"" << (lpName ? lpName : "<unnamed>") << "\"\n";
    }
    shim_SetLastError(0); // ERROR_SUCCESS
    return reinterpret_cast<void*>(0xCAFE0001);
}

int32_t shim_ReleaseMutex(void* hMutex) {
    (void)hMutex;
    if (runtime::get_trace_config().trace_threads) {
        std::cout << "[TRACE:THREADS] ReleaseMutex\n";
    }
    return 1;
}

void* shim_CreateEventA(void* lpEventAttributes, int32_t bManualReset, int32_t bInitialState, const char* lpName) {
    (void)lpEventAttributes;
    (void)bManualReset;
    (void)bInitialState;
    if (runtime::get_trace_config().trace_threads) {
        std::cout << "[TRACE:THREADS] CreateEventA: \"" << (lpName ? lpName : "<unnamed>") << "\"\n";
    }
    return reinterpret_cast<void*>(0xCAFE0002);
}

void shim_Sleep(uint32_t dwMilliseconds) {
    if (runtime::get_trace_config().trace_threads) {
        std::cout << "[TRACE:THREADS] Sleep: " << dwMilliseconds << " ms\n";
    }
    win32compat::sleep(dwMilliseconds);
}

} // namespace pcrecomp::win32
