#include <win32compat/time.h>
#include <pcmodern/time.h>
namespace win32compat {
uint32_t get_tick_count() { return static_cast<uint32_t>(pcmodern::time_ns()/1'000'000); }
uint32_t time_get_time() { return get_tick_count(); }
int32_t query_performance_counter(int64_t* result) {
    if (!result) return 0;
    *result = static_cast<int64_t>(pcmodern::time_ns()); return 1;
}
int32_t query_performance_frequency(int64_t* result) {
    if (!result) return 0;
    *result = 1'000'000'000; return 1;
}
void sleep(uint32_t ms) { pcmodern::sleep_ms(ms); }
}
