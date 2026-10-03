#ifndef PCRECOMP_MODULES_H
#define PCRECOMP_MODULES_H
#include "context.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct PcGuestModule {
    const char* name;
    pc_guest_addr_t image_base;
    uint32_t image_size;
    pc_guest_addr_t entry_point;
} PcGuestModule;
PcStatus pc_register_module(PcRuntime*, const PcGuestModule*);
PcStatus pc_find_module(PcRuntime*, pc_guest_addr_t, PcGuestModule*);
#ifdef __cplusplus
}
#endif
#endif
