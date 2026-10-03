#ifndef PCRECOMP_CALLBACKS_H
#define PCRECOMP_CALLBACKS_H
#include "dispatch.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Caller marshals the guest stack according to the callback signature. */
PcStatus pc_invoke_guest_callback(PcRuntime*, pc_guest_addr_t, PcContext*, pc_guest_addr_t caller);
#ifdef __cplusplus
}
#endif
#endif
