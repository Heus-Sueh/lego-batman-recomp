#ifndef PCRECOMP_MEMORY_H
#define PCRECOMP_MEMORY_H
#include "context.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed arena; owner must outlive binding. NULL/0 unbinds. */
PcStatus pc_bind_memory(PcRuntime*, void* host_base, size_t size);
PcStatus pc_translate(PcRuntime*, pc_guest_addr_t, size_t size, void** result);
PcStatus pc_read8(PcRuntime*, pc_guest_addr_t, uint8_t*);
PcStatus pc_read16(PcRuntime*, pc_guest_addr_t, uint16_t*);
PcStatus pc_read32(PcRuntime*, pc_guest_addr_t, uint32_t*);
PcStatus pc_write8(PcRuntime*, pc_guest_addr_t, uint8_t);
PcStatus pc_write16(PcRuntime*, pc_guest_addr_t, uint16_t);
PcStatus pc_write32(PcRuntime*, pc_guest_addr_t, uint32_t);
#ifdef __cplusplus
}
#endif
#endif
