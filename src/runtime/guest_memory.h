#pragma once

#include "guest_types.h"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include "bridge_adapter.h"
#include <pcrecomp/memory.h>

namespace pcrecomp::runtime {

class GuestMemory {
public:
    static GuestMemory& instance();

    bool initialize();
    void shutdown();

    [[nodiscard]] bool is_initialized() const noexcept { return m_base_ptr != nullptr; }
    [[nodiscard]] uint8_t* base_ptr() const noexcept { return m_base_ptr; }

    [[nodiscard]] uint8_t* translate(GuestAddress addr) const noexcept {
        void* result=nullptr;
        if (!m_base_ptr || pc_translate(bridge_runtime(),addr,1,&result)!=PC_OK) return nullptr;
        return static_cast<uint8_t*>(result);
    }

    [[nodiscard]] void* guest_to_host(GuestAddress addr) const noexcept {
        return translate(addr);
    }

    template <typename T = void>
    [[nodiscard]] T* guest_to_host(GuestAddress addr) const noexcept {
        return reinterpret_cast<T*>(translate(addr));
    }

    template <typename T>
    [[nodiscard]] T* as(GuestAddress addr) const noexcept {
        return reinterpret_cast<T*>(translate(addr));
    }

    template <typename T>
    [[nodiscard]] T read(GuestAddress addr) const noexcept {
        T value{};
        if (const auto* ptr = is_valid(addr,sizeof(T)) ? translate(addr) : nullptr) {
            std::memcpy(&value, ptr, sizeof(T));
        }
        return value;
    }

    template <typename T>
    void write(GuestAddress addr, const T& value) const noexcept {
        if (auto* ptr = is_valid(addr,sizeof(T)) ? translate(addr) : nullptr) {
            std::memcpy(ptr, &value, sizeof(T));
        }
    }

    // Section 19: Guest Memory Access Helpers
    [[nodiscard]] uint8_t  read8(guest_addr_t addr) const noexcept { return read<uint8_t>(addr); }
    [[nodiscard]] uint16_t read16(guest_addr_t addr) const noexcept { return read<uint16_t>(addr); }
    [[nodiscard]] uint32_t read32(guest_addr_t addr) const noexcept { return read<uint32_t>(addr); }
    [[nodiscard]] uint64_t read64(guest_addr_t addr) const noexcept { return read<uint64_t>(addr); }
    [[nodiscard]] float    read_float(guest_addr_t addr) const noexcept { return read<float>(addr); }
    [[nodiscard]] double   read_double(guest_addr_t addr) const noexcept { return read<double>(addr); }

    void write8(guest_addr_t addr, uint8_t val) const noexcept { write<uint8_t>(addr, val); }
    void write16(guest_addr_t addr, uint16_t val) const noexcept { write<uint16_t>(addr, val); }
    void write32(guest_addr_t addr, uint32_t val) const noexcept { write<uint32_t>(addr, val); }
    void write64(guest_addr_t addr, uint64_t val) const noexcept { write<uint64_t>(addr, val); }
    void write_float(guest_addr_t addr, float val) const noexcept { write<float>(addr, val); }
    void write_double(guest_addr_t addr, double val) const noexcept { write<double>(addr, val); }

    [[nodiscard]] bool is_valid(GuestAddress addr, size_t size = 1) const noexcept;

    // Compatibility allocations use a separate arena above the Win32 heap.
    static constexpr GuestAddress allocation_begin = 0x20000000;
    static constexpr GuestAddress allocation_end = 0x30000000;
    GuestAddress guest_alloc(size_t size);
    void guest_free(GuestAddress addr);

private:
    GuestMemory();
    ~GuestMemory();

    uint8_t*     m_base_ptr{nullptr};
    size_t       m_allocated_size{0};
    bool         m_fixed_base{false};
    GuestAddress m_heap_break{allocation_begin};
};

inline void* guest_to_host(GuestAddress addr) noexcept {
    return GuestMemory::instance().guest_to_host(addr);
}

} // namespace pcrecomp::runtime
