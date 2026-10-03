#include <pcmodern/time.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <iostream>
namespace pcmodern {
namespace {
HostCallbacks backend;
std::mutex lock;
HostCallbacks snapshot() { std::lock_guard<std::mutex> guard(lock); return backend; }
}
void set_host_callbacks(HostCallbacks value) { std::lock_guard<std::mutex> guard(lock); backend = value; }
uint64_t time_ns() {
    auto b = snapshot();
    if (b.time_ns) return b.time_ns(b.user);
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
void sleep_ms(uint32_t ms) {
    auto b = snapshot();
    if (b.sleep_ms) b.sleep_ms(b.user,ms);
    else std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
void log(std::string_view category, std::string_view message) {
    auto b = snapshot();
    if (b.log) b.log(b.user,category,message);
    else std::clog << '[' << category << "] " << message << '\n';
}
}
