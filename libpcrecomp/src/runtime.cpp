#include <pcrecomp/memory.h>
#include <pcrecomp/imports.h>
#include <pcrecomp/callbacks.h>
#include <pcrecomp/modules.h>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <deque>
#include <new>

struct Binding { PcGuestFunction fn; void* user; };
struct Module { std::string name; PcGuestModule value; };
struct PcRuntime {
    PcRuntimeConfig config{};
    std::string application, root, user_root;
    uint8_t* memory{};
    size_t size{};
    std::unordered_map<uint32_t, Binding> functions, imports;
    std::deque<Module> modules;
    PcFault fault{};
};
static PcStatus report(PcRuntime* r, PcStatus status, const char* category,
                       uint32_t caller, uint32_t target, const PcContext* ctx = nullptr) {
    if (!r) return PC_INVALID_ARGUMENT;
    r->fault = {status, category, caller, target, ctx ? *ctx : PcContext{}};
    if (r->config.diagnostic) r->config.diagnostic(r->config.diagnostic_user, &r->fault);
    else if (status != PC_OK) {
        const auto& c = r->fault.context;
        std::fprintf(stderr, "[libpcrecomp:%s] status=%d caller=%08x target=%08x eax=%08x ecx=%08x edx=%08x ebx=%08x esp=%08x ebp=%08x esi=%08x edi=%08x flags=%08x\n",
            category, int(status), caller, target, c.eax, c.ecx, c.edx, c.ebx,
            c.esp, c.ebp, c.esi, c.edi, c.eflags);
    }
    return status;
}
extern "C" PcRuntime* pc_runtime_create(const PcRuntimeConfig* config) {
    try {
        auto* r = new PcRuntime;
        try {
            if (config) r->config = *config;
            r->application = r->config.application_name ? r->config.application_name : "";
            r->root = r->config.game_root ? r->config.game_root : "";
            r->user_root = r->config.user_data_root ? r->config.user_data_root : "";
            r->config.application_name = r->application.c_str();
            r->config.game_root = r->root.c_str();
            r->config.user_data_root = r->user_root.c_str();
        } catch (...) { delete r; throw; }
        return r;
    } catch (...) { return nullptr; }
}
extern "C" void pc_runtime_destroy(PcRuntime* r) { delete r; }
extern "C" const PcFault* pc_last_fault(const PcRuntime* r) { return r ? &r->fault : nullptr; }
extern "C" PcStatus pc_bind_memory(PcRuntime* r, void* base, size_t size) {
    if (!r || ((base == nullptr) != (size == 0)) || uint64_t(size) > (uint64_t(1) << 32)) return PC_INVALID_ARGUMENT;
    r->memory = static_cast<uint8_t*>(base); r->size = size; return PC_OK;
}
extern "C" PcStatus pc_translate(PcRuntime* r, uint32_t addr, size_t size, void** out) {
    if (!r || !out) return PC_INVALID_ARGUMENT;
    *out = nullptr;
    if (!r->memory || addr > r->size || size > r->size - addr)
        return report(r, PC_MEMORY_BOUNDS, "memory", 0, addr);
    *out = r->memory + addr; return PC_OK;
}
template<class T> static PcStatus read(PcRuntime* r, uint32_t addr, T* value) {
    if (!value) return PC_INVALID_ARGUMENT;
    void* p; auto status = pc_translate(r, addr, sizeof(T), &p);
    if (status != PC_OK) return status;
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i) result |= T(static_cast<uint8_t*>(p)[i]) << (i * 8);
    *value = result; return PC_OK;
}
template<class T> static PcStatus write(PcRuntime* r, uint32_t addr, T value) {
    void* p; auto status = pc_translate(r, addr, sizeof(T), &p);
    if (status != PC_OK) return status;
    for (size_t i = 0; i < sizeof(T); ++i) static_cast<uint8_t*>(p)[i] = uint8_t(value >> (i * 8));
    return PC_OK;
}
#define PC_MEMORY_API(N, T) \
extern "C" PcStatus pc_read##N(PcRuntime* r, uint32_t a, T* v) { return read(r,a,v); } \
extern "C" PcStatus pc_write##N(PcRuntime* r, uint32_t a, T v) { return write(r,a,v); }
PC_MEMORY_API(8, uint8_t)
PC_MEMORY_API(16, uint16_t)
PC_MEMORY_API(32, uint32_t)
static PcStatus bind(std::unordered_map<uint32_t, Binding>& map, uint32_t a, PcGuestFunction fn, void* user) {
    if (!fn) return PC_INVALID_ARGUMENT;
    try { return map.emplace(a, Binding{fn,user}).second ? PC_OK : PC_DUPLICATE; }
    catch (...) { return PC_INTERNAL_ERROR; }
}
extern "C" PcStatus pc_register_guest_function(PcRuntime* r, uint32_t a, PcGuestFunction f, void* u) {
    return r ? bind(r->functions,a,f,u) : PC_INVALID_ARGUMENT;
}
extern "C" PcStatus pc_register_import(PcRuntime* r, uint32_t a, PcGuestFunction f, void* u) {
    return r ? bind(r->imports,a,f,u) : PC_INVALID_ARGUMENT;
}
static PcStatus call(PcRuntime* r, uint32_t a, PcContext* c, uint32_t caller, const char* category, bool imports, bool trace) {
    if (!r || !c) return PC_INVALID_ARGUMENT;
    auto& map = imports ? r->imports : r->functions;
    auto it = map.find(a);
    if (it == map.end()) return report(r, PC_UNKNOWN_TARGET, category, caller, a, c);
    const Binding binding = it->second;
    if (trace) report(r, PC_OK, category, caller, a, c);
    PcStatus status;
    try { status = binding.fn(r,c,binding.user); }
    catch (...) { status = PC_INTERNAL_ERROR; }
    return (status == PC_OK || status == PC_YIELDED) ? status : report(r,status,category,caller,a,c);
}
extern "C" PcStatus pc_call_guest(PcRuntime* r,uint32_t a,PcContext* c,uint32_t caller) { return call(r,a,c,caller,"dispatch",false,false); }
extern "C" PcStatus pc_call_indirect(PcRuntime* r,uint32_t a,PcContext* c,uint32_t caller) { return call(r,a,c,caller,"icall",false,r && r->config.trace_icalls); }
extern "C" PcStatus pc_unknown_indirect(PcRuntime* r,uint32_t a,const PcContext* c,uint32_t caller) {
    if (!r || !c) return PC_INVALID_ARGUMENT;
    return report(r,PC_UNKNOWN_TARGET,"icall",caller,a,c);
}
extern "C" PcStatus pc_call_import(PcRuntime* r,uint32_t a,PcContext* c,uint32_t caller) { return call(r,a,c,caller,"import",true,r && r->config.trace_imports); }
extern "C" PcStatus pc_invoke_guest_callback(PcRuntime* r,uint32_t a,PcContext* c,uint32_t caller) { return call(r,a,c,caller,"callback",false,r && r->config.trace_callbacks); }
extern "C" PcStatus pc_register_module(PcRuntime* r,const PcGuestModule* m) {
    if (!r || !m || !m->name || !m->image_size) return PC_INVALID_ARGUMENT;
    uint64_t end = uint64_t(m->image_base) + m->image_size;
    if (end > (uint64_t(1)<<32) || m->entry_point < m->image_base || m->entry_point >= end) return PC_INVALID_ARGUMENT;
    for (const auto& old : r->modules)
        if (m->image_base < uint64_t(old.value.image_base)+old.value.image_size && old.value.image_base < end) return PC_DUPLICATE;
    try {
        r->modules.push_back({m->name,*m});
        r->modules.back().value.name = r->modules.back().name.c_str();
        return PC_OK;
    } catch (...) { return PC_INTERNAL_ERROR; }
}
extern "C" PcStatus pc_find_module(PcRuntime* r,uint32_t a,PcGuestModule* out) {
    if (!r || !out) return PC_INVALID_ARGUMENT;
    for (const auto& m : r->modules) if (a >= m.value.image_base && uint64_t(a) < uint64_t(m.value.image_base)+m.value.image_size) { *out=m.value; return PC_OK; }
    return PC_UNKNOWN_TARGET;
}
