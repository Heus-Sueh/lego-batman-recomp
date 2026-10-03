#pragma once

#include <cstdint>
#include <cstddef>

namespace pcrecomp::win32 {

// Guest memory addresses for Direct3D 9 COM sentinels and VTables
constexpr uint32_t GUEST_D3D9_VTABLE_ADDR        = 0x00C97000u;
constexpr uint32_t GUEST_D3D9DEVICE_VTABLE_ADDR  = 0x00C97100u;
constexpr uint32_t GUEST_D3D9_INSTANCE_ADDR      = 0x00C97400u;
constexpr uint32_t GUEST_D3D9DEVICE_INSTANCE_ADDR= 0x00C97410u;

constexpr uint32_t GUEST_D3D9_METHOD_BASE        = 0x008E8000u;
constexpr uint32_t GUEST_D3D9DEVICE_METHOD_BASE  = 0x008E8200u;

// Initialize and shutdown Direct3D 9 subsystem (DXVK-Native backend)
bool init_d3d9_subsystem();
void shutdown_d3d9_subsystem();

// Direct3DCreate9 shim: returns 32-bit guest pointer to IDirect3D9 wrapper (0x00C97400)
uint32_t shim_Direct3DCreate9(uint32_t sdk_version);

// Thunk dispatchers called from recomp_dispatch.cpp
void d3d9_thunk_dispatch(uint32_t method_idx);
void d3d9_device_thunk_dispatch(uint32_t method_idx);

} // namespace pcrecomp::win32
