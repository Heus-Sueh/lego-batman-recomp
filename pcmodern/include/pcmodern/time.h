#pragma once
#include <cstdint>
#include <string_view>
namespace pcmodern {
struct HostCallbacks {
    void* user{};
    uint64_t (*time_ns)(void*){};
    void (*sleep_ms)(void*, uint32_t){};
    void (*log)(void*, std::string_view category, std::string_view message){};
};
// Configure before execution; callbacks/user must outlive their installation.
// Passing {} restores the native monotonic clock and host sleep/log services.
void set_host_callbacks(HostCallbacks callbacks);
uint64_t time_ns();
void sleep_ms(uint32_t duration);
void log(std::string_view category, std::string_view message);
}
