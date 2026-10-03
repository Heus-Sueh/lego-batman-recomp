#ifndef PCRECOMP_CONTEXT_H
#define PCRECOMP_CONTEXT_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef uint32_t pc_guest_addr_t;
/* ABI v1: integer registers only. Legacy x87/SSE state remains in its adapter. */
typedef struct PcContext {
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi, eflags;
} PcContext;
typedef struct PcRuntime PcRuntime;
typedef enum PcStatus {
    PC_OK = 0, PC_INVALID_ARGUMENT, PC_MEMORY_BOUNDS, PC_UNKNOWN_TARGET,
    PC_DUPLICATE, PC_INTERNAL_ERROR, PC_YIELDED
} PcStatus;
typedef struct PcFault {
    PcStatus status;
    const char* category;
    pc_guest_addr_t caller, target;
    PcContext context;
} PcFault;
typedef void (*PcDiagnostic)(void* user, const PcFault* fault);
typedef struct PcRuntimeConfig {
    const char* application_name;
    const char* game_root;
    const char* user_data_root;
    int trace_imports, trace_icalls, trace_callbacks;
    PcDiagnostic diagnostic;
    void* diagnostic_user;
} PcRuntimeConfig;
PcRuntime* pc_runtime_create(const PcRuntimeConfig* config);
void pc_runtime_destroy(PcRuntime* runtime);
const PcFault* pc_last_fault(const PcRuntime* runtime);
#ifdef __cplusplus
}
#endif
#endif
