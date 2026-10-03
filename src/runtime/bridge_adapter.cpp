#include "bridge_adapter.h"
#include "diagnostics.h"
#include "dispatch.h"
#include "target_config.h"
#include <pcrecomp/win32_time.h>
#include <pcrecomp/win32_fs.h>
#include <pcmodern/time.h>
#include <memory>
#include <stdexcept>
#include <cstdio>
extern "C" {
extern uint32_t g_eax,g_ecx,g_edx,g_ebx,g_esp,g_ebp,g_esi,g_edi;
extern uint32_t g_cur_func;
}
namespace pcrecomp::runtime {
PcRuntime* bridge_runtime() {
    static const auto runtime = [] {
        PcRuntimeConfig config{};
        config.application_name=pcrecomp::target::title;
        config.diagnostic=[](void*,const PcFault* f) {
            char text[320];
            std::snprintf(text,sizeof(text),"status=%d caller=%08x target=%08x eax=%08x ecx=%08x edx=%08x esp=%08x ebx=%08x ebp=%08x esi=%08x edi=%08x eflags=%08x",
                int(f->status),f->caller,f->target,f->context.eax,f->context.ecx,f->context.edx,f->context.esp,
                f->context.ebx,f->context.ebp,f->context.esi,f->context.edi,f->context.eflags);
            pcmodern::log(f->category,text);
        };
        std::unique_ptr<PcRuntime,decltype(&pc_runtime_destroy)> r(pc_runtime_create(&config),pc_runtime_destroy);
        if (!r || pc_register_time_imports(r.get()) != PC_OK || pc_register_fs_imports(r.get()) != PC_OK)
            throw std::runtime_error("Runtime bridge initialization failed");
        return r;
    }();
    return runtime.get();
}
}
extern "C" void gta_call_time_import(uint32_t id) {
    PcContext context{g_eax,g_ecx,g_edx,g_ebx,g_esp,g_ebp,g_esi,g_edi,0};
    auto status=pc_call_import(pcrecomp::runtime::bridge_runtime(),id,&context,g_cur_func);
    if (status != PC_OK) throw std::runtime_error("Time import bridge failed");
    g_eax=context.eax; g_esp=context.esp;
}

extern "C" void gta_call_fs_import(uint32_t id) {
    PcContext context{g_eax,g_ecx,g_edx,g_ebx,g_esp,g_ebp,g_esi,g_edi,0};
    auto status=pc_call_import(pcrecomp::runtime::bridge_runtime(),id,&context,g_cur_func);
    if (status != PC_OK) throw std::runtime_error("FS import bridge failed");
    g_eax=context.eax; g_esp=context.esp;
}

extern "C" void recomp_runtime_unresolved(uint32_t target,uint32_t caller) {
    PcContext context{g_eax,g_ecx,g_edx,g_ebx,g_esp,g_ebp,g_esi,g_edi,0};
    pc_unknown_indirect(pcrecomp::runtime::bridge_runtime(),target,&context,caller);
    pcrecomp::runtime::DispatchTable::instance().log_unresolved(caller,target,context,"generated_dispatch");
    pcrecomp::runtime::ExecutionFrontier::instance().record_failure(caller,target,"Unmapped indirect call","UNKNOWN");
    throw std::runtime_error("Unmapped indirect call: execution stopped");
}
