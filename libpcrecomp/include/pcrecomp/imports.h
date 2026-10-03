#ifndef PCRECOMP_IMPORTS_H
#define PCRECOMP_IMPORTS_H
#include "dispatch.h"
#ifdef __cplusplus
extern "C" {
#endif
PcStatus pc_register_import(PcRuntime*, uint32_t id, PcGuestFunction, void* user);
PcStatus pc_call_import(PcRuntime*, uint32_t id, PcContext*, pc_guest_addr_t caller);
#ifdef __cplusplus
}
#endif
#endif
