#ifndef PCRECOMP_DISPATCH_H
#define PCRECOMP_DISPATCH_H
#include "context.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Handlers must return explicit failure and must not throw across this ABI. */
typedef PcStatus (*PcGuestFunction)(PcRuntime*, PcContext*, void* user);
PcStatus pc_register_guest_function(PcRuntime*, pc_guest_addr_t, PcGuestFunction, void* user);
PcStatus pc_call_guest(PcRuntime*, pc_guest_addr_t, PcContext*, pc_guest_addr_t caller);
PcStatus pc_call_indirect(PcRuntime*, pc_guest_addr_t, PcContext*, pc_guest_addr_t caller);
/* For legacy resolvers during migration: report failure without invoking code. */
PcStatus pc_unknown_indirect(PcRuntime*, pc_guest_addr_t, const PcContext*, pc_guest_addr_t caller);
#ifdef __cplusplus
}
#endif
#endif
