#pragma once
#include <cstdint>
namespace win32compat {
uint32_t get_tick_count();
uint32_t time_get_time();
int32_t query_performance_counter(int64_t* result);
int32_t query_performance_frequency(int64_t* result);
void sleep(uint32_t milliseconds);
}
