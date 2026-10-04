#pragma once
#include <cstdint>
#include <map>
#include <mutex>

namespace win32compat {
// Borrowed arena. All public addresses are guest offsets, never host pointers.
// Adapted from pcrecomp win32hle's HeapAlloc/Free/ReAlloc boundary semantics.
class GuestHeap {
public:
    static constexpr uint32_t process_heap = 0x10000;
    GuestHeap(uint8_t* base, uint32_t begin, uint32_t end);
    uint32_t create(uint32_t flags, uint32_t initial, uint32_t maximum);
    bool destroy(uint32_t heap);
    uint32_t alloc(uint32_t heap, uint32_t flags, uint32_t size);
    bool free(uint32_t heap, uint32_t address);
    uint32_t size(uint32_t heap, uint32_t address);
    uint32_t realloc(uint32_t heap, uint32_t flags, uint32_t address, uint32_t size);
private:
    struct Block { uint32_t heap, size, capacity; };
    struct Heap { uint32_t maximum; uint64_t used; };
    uint32_t allocate(uint32_t heap, uint32_t flags, uint32_t size);
    void release(std::map<uint32_t, Block>::iterator block);
    uint8_t* base_;
    uint32_t begin_, end_, next_heap_ = 0x20000;
    std::map<uint32_t, Block> blocks_;
    std::map<uint32_t, Heap> heaps_;
    std::mutex mutex_;
};
}
