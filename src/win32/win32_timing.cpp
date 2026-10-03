#include "win32_shims.h"
#include <win32compat/time.h>

namespace pcrecomp::win32 {

uint32_t shim_timeGetTime() {
    return win32compat::time_get_time();
}

uint32_t shim_GetTickCount() {
    return win32compat::get_tick_count();
}

int32_t shim_QueryPerformanceCounter(int64_t* lpPerformanceCount) {
    return win32compat::query_performance_counter(lpPerformanceCount);
}

int32_t shim_QueryPerformanceFrequency(int64_t* lpFrequency) {
    return win32compat::query_performance_frequency(lpFrequency);
}

} // namespace pcrecomp::win32
