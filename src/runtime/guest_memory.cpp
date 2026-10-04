#include "guest_memory.h"
#include "guest_heap.h"
#include "bridge_adapter.h"
#include <pcrecomp/memory.h>
#include <pcmodern/memory.h>
#include <iostream>

extern "C" ptrdiff_t g_mem_base;

namespace pcrecomp::runtime {
GuestMemory::GuestMemory() { (void)bridge_runtime(); }

GuestMemory& GuestMemory::instance() {
    static GuestMemory s_instance;
    return s_instance;
}

GuestMemory::~GuestMemory() {
    shutdown();
}

bool GuestMemory::initialize() {
    if (m_base_ptr) {
        return true;
    }

    m_allocated_size = GUEST_ADDRESS_SPACE_SIZE;

    m_base_ptr = static_cast<uint8_t*>(pcmodern::allocate_memory(m_allocated_size));

    if (!m_base_ptr) {
        std::cerr << "[GuestMemory] Failed to initialize 2GB guest memory space.\n";
        return false;
    }

    g_mem_base = reinterpret_cast<ptrdiff_t>(m_base_ptr);
    if (pc_bind_memory(bridge_runtime(),m_base_ptr,m_allocated_size) != PC_OK) {
        shutdown();
        return false;
    }

    std::cout << "[GuestMemory] Allocated 2GB guest address space at host "
              << static_cast<void*>(m_base_ptr) << "\n";
    return true;
}

void GuestMemory::shutdown() {
    reset_guest_heap();
    m_heap_break = allocation_begin;
    if (!m_base_ptr) return;

    g_mem_base = 0;
    pc_bind_memory(bridge_runtime(),nullptr,0);

    pcmodern::release_memory(m_base_ptr,m_allocated_size);

    m_base_ptr = nullptr;
    m_allocated_size = 0;
}

bool GuestMemory::is_valid(GuestAddress addr, size_t size) const noexcept {
    if (!m_base_ptr) return false;
    if (addr > m_allocated_size || size > m_allocated_size - addr) return false;
    return true;
}

GuestAddress GuestMemory::guest_alloc(size_t size) {
    if (!m_base_ptr || size == 0) {
        return 0;
    }
    // Check before alignment to reject overflow and keep allocations in the arena.
    if (m_heap_break >= allocation_end || size > allocation_end - m_heap_break)
        return 0;
    // 8-byte alignment
    size = (size + 7) & ~static_cast<size_t>(7);
    GuestAddress alloc_addr = m_heap_break;
    if (static_cast<uint64_t>(alloc_addr) + size > allocation_end) {
        std::cerr << "[GuestMemory] Out of guest memory on allocation of " << size << " bytes.\n";
        return 0;
    }
    m_heap_break += static_cast<GuestAddress>(size);
    std::memset(m_base_ptr + alloc_addr, 0, size);
    return alloc_addr;
}

void GuestMemory::guest_free(GuestAddress addr) {
    (void)addr; // Bump allocator; freed on shutdown
}

} // namespace pcrecomp::runtime
