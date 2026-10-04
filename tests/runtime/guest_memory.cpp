#include "../../src/runtime/guest_memory.h"
#include "../../src/runtime/guest_heap.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed: " #x << '\n'; std::exit(1); } } while (0)
extern "C" { ptrdiff_t g_mem_base = 0; }
namespace pcrecomp::runtime {
PcRuntime* bridge_runtime() {
    static PcRuntime* runtime = pc_runtime_create(nullptr);
    return runtime;
}
}
int main(int argc, char**) {
    using namespace pcrecomp::runtime;
    auto& memory = GuestMemory::instance();
    CHECK(memory.initialize());
    memory.write32(0x02000000, 0x12345678); // Synthetic PE data at the former arena.
    CHECK(!memory.guest_alloc(SIZE_MAX));
    CHECK(!memory.guest_alloc(0));
    CHECK(!memory.guest_alloc(GuestMemory::allocation_end - GuestMemory::allocation_begin + 1ull));
    const auto allocation = memory.guest_alloc(17);
    // An extra argument deliberately selects the obsolete origin as a negative control.
    CHECK(allocation == (argc > 1 ? 0x02000000u : GuestMemory::allocation_begin));
    CHECK(memory.read32(0x02000000) == 0x12345678);
    CHECK(memory.guest_alloc(1) == allocation + 24);
    const auto heap_block = guest_heap().alloc(win32compat::GuestHeap::process_heap, 8, 16);
    CHECK(heap_block == guest_heap_begin && heap_block + 16 <= allocation);
    memory.shutdown();
    CHECK(!memory.guest_alloc(1));
    CHECK(memory.initialize());
    CHECK(memory.guest_alloc(1) == GuestMemory::allocation_begin);
    CHECK(guest_heap().alloc(win32compat::GuestHeap::process_heap, 0, 16) == guest_heap_begin);
    memory.shutdown();
}
