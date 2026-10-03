#pragma once
#include <cstddef>
namespace pcmodern {
// Anonymous zero-filled host storage. Guest address policy belongs to caller.
void* allocate_memory(size_t size);
void release_memory(void* address, size_t size);
}
