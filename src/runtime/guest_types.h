#pragma once

#include <cstdint>
#include <cstddef>
#include <pcrecomp/context.h>

namespace pcrecomp::runtime {

// Explicit 32-bit guest address type (gta_sa.exe is an x86-32 binary)
using GuestAddress = pc_guest_addr_t;
using guest_addr_t = pc_guest_addr_t;

// Standard PE memory layout constants for GTA: San Andreas
constexpr GuestAddress GUEST_IMAGE_BASE = 0x00400000;
constexpr GuestAddress GUEST_CODE_START = 0x00401000;
constexpr GuestAddress GUEST_CODE_END   = 0x00856CDA;
constexpr size_t       GUEST_ADDRESS_SPACE_SIZE = 0x80000000; // 2GB virtual space

// x86-32 General-purpose register context representation
using GuestRegisters = PcContext;

} // namespace pcrecomp::runtime
