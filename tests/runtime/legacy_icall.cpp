#include <pcrecomp/dispatch.h>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include "../../thirdparty/pcrecomp/runtime/recomp32/recomp_types.h"
extern "C" {
uint32_t g_eax=17,g_ecx=18,g_edx=19,g_esp=32,g_ebx=0,g_ebp=0,g_esi=0,g_edi=0;
uint32_t g_cur_func=0x120,g_icall_trace[ICALL_TRACE_SIZE]{},g_icall_trace_idx=0,g_icall_count=0;
ptrdiff_t g_mem_base=0;
recomp_func_t recomp_lookup(uint32_t) { return nullptr; }
recomp_func_t recomp_lookup_manual(uint32_t) { return nullptr; }
recomp_func_t recomp_lookup_import(uint32_t) { return nullptr; }
}
static PcRuntime* runtime;
extern "C" void recomp_runtime_unresolved(uint32_t target,uint32_t caller) {
    PcContext ctx{}; ctx.eax=g_eax; ctx.esp=g_esp;
    pc_unknown_indirect(runtime,target,&ctx,caller);
    throw std::runtime_error("expected unresolved target");
}
int main() {
    uint32_t& esp=g_esp;
    runtime=pc_runtime_create(nullptr);
    if (!runtime) return 1;
    unsigned failures=0;
    try { RECOMP_ICALL(0x200); } catch (const std::runtime_error&) { ++failures; }
    try { RECOMP_ITAIL(0x201); } catch (const std::runtime_error&) { ++failures; }
    const PcFault* fault=pc_last_fault(runtime);
    const bool passed=failures==2 && fault->target==0x201 && fault->caller==0x120 &&
        fault->context.eax==17 && fault->context.esp==32 && g_eax==17 && g_esp==32;
    pc_runtime_destroy(runtime);
    if (!passed) std::fprintf(stderr,"Unknown targets silently continued or mutated guest state\n");
    return passed ? 0 : 1;
}
