#include <win32compat/heap.h>
#include <array>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed: " #x << '\n'; std::exit(1); } } while (0)
int main() {
    std::array<uint8_t, 512> memory;
    memory.fill(0xAA);
    win32compat::GuestHeap heap(memory.data(), 16, 496);
    const auto h = win32compat::GuestHeap::process_heap;
    const auto a = heap.alloc(h, 8, 17);
    CHECK(a == 16 && heap.size(h, a) == 17);
    CHECK(std::all_of(memory.begin() + a, memory.begin() + a + 17, [](auto b) { return b == 0; }));
    memory[a] = 42;
    CHECK(!heap.alloc(h, 0, UINT32_MAX));
    CHECK(!heap.alloc(123, 0, 16));
    CHECK(!heap.alloc(h, 4, 16));
    CHECK(!heap.free(123, a));
    CHECK(heap.size(h, a + 1) == UINT32_MAX);
    CHECK(!heap.realloc(h, 0x10, a, 80));
    CHECK(!heap.realloc(h, 0, a, 512));
    CHECK(heap.size(h, a) == 17 && memory[a] == 42);
    const auto b = heap.realloc(h, 8, a, 80);
    CHECK(b && b != a && memory[b] == 42);
    CHECK(heap.size(h, a) == UINT32_MAX);
    CHECK(std::all_of(memory.begin() + b + 17, memory.begin() + b + 80, [](auto v) { return v == 0; }));
    CHECK(heap.realloc(h, 0, b, 4) == b);
    CHECK(heap.realloc(h, 8, b, 60) == b && memory[b + 4] == 0);
    CHECK(heap.free(h, b) && !heap.free(h, b));
    CHECK(heap.alloc(h, 0, 0) == a);
    const auto custom = heap.create(0, 0, 32);
    const auto c = heap.alloc(custom, 0, 32);
    CHECK(custom && c && !heap.alloc(custom, 0, 1));
    CHECK(!heap.free(h, c));
    CHECK(!heap.destroy(h) && heap.destroy(custom));
    CHECK(!heap.destroy(custom) && heap.size(custom, c) == UINT32_MAX);
    CHECK(!heap.create(0, 64, 32));
    CHECK(heap.free(h, a));
    CHECK(heap.alloc(h, 0, 480) == 16);
    CHECK(!heap.alloc(h, 0, 1));
    CHECK(memory.front() == 0xAA && memory.back() == 0xAA);
    std::cout << "Guest heap bounds, ownership, reuse and realloc checks passed\n";
}
