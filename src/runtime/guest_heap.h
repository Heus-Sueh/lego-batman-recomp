#pragma once
#include <win32compat/heap.h>
namespace pcrecomp::runtime {
inline constexpr uint32_t guest_heap_begin = 0x10000000;
inline constexpr uint32_t guest_heap_end = 0x20000000;
win32compat::GuestHeap& guest_heap();
void reset_guest_heap();
}
