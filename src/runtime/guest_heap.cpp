#include "guest_heap.h"
#include "guest_memory.h"
#include <memory>
#include <mutex>
#include <stdexcept>

namespace pcrecomp::runtime {
namespace {
std::unique_ptr<win32compat::GuestHeap> heap;
std::mutex lifecycle_mutex;
}
win32compat::GuestHeap& guest_heap() {
    std::lock_guard<std::mutex> lock(lifecycle_mutex);
    auto& memory = GuestMemory::instance();
    // Preserve the existing generated allocator's arena; outside the mapped PE.
    constexpr uint32_t begin = guest_heap_begin, end = guest_heap_end;
    if (!memory.is_initialized() || !memory.is_valid(begin, end - begin))
        throw std::runtime_error("Guest heap arena unavailable");
    if (!heap) heap = std::make_unique<win32compat::GuestHeap>(memory.base_ptr(), begin, end);
    return *heap;
}
// Shutdown requires guest execution to have stopped.
void reset_guest_heap() {
    std::lock_guard<std::mutex> lock(lifecycle_mutex);
    heap.reset();
}
}
