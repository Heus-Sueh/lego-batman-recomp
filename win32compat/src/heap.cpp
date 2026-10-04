// Selective adaptation of pcrecomp runtime/win32hle/kernel32.c.
// Upstream attribution and MIT license: docs/references/win32hle-LICENSE.txt.
#include <win32compat/heap.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace win32compat {
GuestHeap::GuestHeap(uint8_t* base, uint32_t begin, uint32_t end)
    : base_(base), begin_(begin), end_(end) {
    if (!base || !begin || begin >= end || (begin & 15))
        throw std::invalid_argument("Invalid guest heap arena");
    heaps_.emplace(process_heap, Heap{0, 0});
}
uint32_t GuestHeap::create(uint32_t flags, uint32_t initial, uint32_t maximum) {
    std::lock_guard<std::mutex> lock(mutex_);
    if ((flags & ~1u) || (maximum && initial > maximum) || initial > end_ - begin_ ||
        next_heap_ == UINT32_MAX) return 0;
    const auto handle = next_heap_++;
    heaps_.emplace(handle, Heap{maximum, 0});
    return handle;
}
void GuestHeap::release(std::map<uint32_t, Block>::iterator block) {
    heaps_.at(block->second.heap).used -= block->second.capacity;
    blocks_.erase(block);
}
bool GuestHeap::destroy(uint32_t heap) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (heap == process_heap || !heaps_.count(heap)) return false;
    for (auto it = blocks_.begin(); it != blocks_.end();) {
        if (it->second.heap == heap) { auto old = it++; release(old); }
        else ++it;
    }
    heaps_.erase(heap);
    return true;
}
uint32_t GuestHeap::allocate(uint32_t heap, uint32_t flags, uint32_t size) {
    auto h = heaps_.find(heap);
    if (h == heaps_.end() || (flags & ~9u)) return 0;
    const uint64_t capacity = (uint64_t(std::max(size, 1u)) + 15u) & ~uint64_t(15);
    if (capacity > end_ - begin_ ||
        (h->second.maximum && h->second.used + capacity > h->second.maximum)) return 0;
    uint64_t address = begin_;
    for (const auto& [start, block] : blocks_) {
        if (address + capacity <= start) break;
        address = uint64_t(start) + block.capacity;
    }
    if (address + capacity > end_) return 0;
    try {
        blocks_.emplace(uint32_t(address), Block{heap, size, uint32_t(capacity)});
    } catch (const std::bad_alloc&) {
        return 0;
    }
    h->second.used += capacity;
    if (flags & 8) std::memset(base_ + address, 0, size);
    return uint32_t(address);
}
uint32_t GuestHeap::alloc(uint32_t heap, uint32_t flags, uint32_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    return allocate(heap, flags, size);
}
bool GuestHeap::free(uint32_t heap, uint32_t address) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!heaps_.count(heap)) return false;
    if (!address) return true;
    auto block = blocks_.find(address);
    if (block == blocks_.end() || block->second.heap != heap) return false;
    release(block);
    return true;
}
uint32_t GuestHeap::size(uint32_t heap, uint32_t address) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto block = blocks_.find(address);
    return block != blocks_.end() && block->second.heap == heap ? block->second.size : UINT32_MAX;
}
uint32_t GuestHeap::realloc(uint32_t heap, uint32_t flags, uint32_t address, uint32_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto old = blocks_.find(address);
    if (old == blocks_.end() || old->second.heap != heap || (flags & ~0x19u)) return 0;
    if (size <= old->second.capacity) {
        if ((flags & 8) && size > old->second.size)
            std::memset(base_ + address + old->second.size, 0, size - old->second.size);
        old->second.size = size;
        return address;
    }
    if (flags & 0x10) return 0; // HEAP_REALLOC_IN_PLACE_ONLY
    // Preserve the old block on failure, as required by HeapReAlloc.
    auto& owner = heaps_.at(heap);
    owner.used -= old->second.capacity;
    const auto replacement = allocate(heap, flags & 9u, size);
    owner.used += old->second.capacity;
    if (!replacement) return 0;
    std::memcpy(base_ + replacement, base_ + address, old->second.size);
    release(old);
    return replacement;
}
}
