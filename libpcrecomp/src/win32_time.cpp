#include <pcrecomp/win32_time.h>
#include <pcrecomp/memory.h>
#include <win32compat/time.h>
namespace {
PcStatus ticks(PcRuntime*, PcContext* c, void*) {
    c->eax = win32compat::get_tick_count(); c->esp += 4; return PC_OK;
}
PcStatus multimedia_ticks(PcRuntime*, PcContext* c, void*) {
    c->eax = win32compat::time_get_time(); c->esp += 4; return PC_OK;
}
PcStatus query(PcRuntime* r, PcContext* c, bool frequency) {
    uint32_t address;
    if (c->esp > UINT32_MAX - 4) return PC_MEMORY_BOUNDS;
    auto status = pc_read32(r,c->esp+4,&address);
    if (status != PC_OK) return status;
    if (!address) { c->eax=0; c->esp+=8; return PC_OK; }
    void* p;
    status = pc_translate(r,address,8,&p);
    if (status != PC_OK) return status;
    int64_t value;
    if (frequency) win32compat::query_performance_frequency(&value);
    else win32compat::query_performance_counter(&value);
    auto* bytes = static_cast<uint8_t*>(p);
    for (unsigned i=0; i<8; ++i) bytes[i]=uint8_t(uint64_t(value)>>(i*8));
    c->eax=1; c->esp+=8; return PC_OK;
}
PcStatus counter(PcRuntime* r,PcContext* c,void*) { return query(r,c,false); }
PcStatus frequency(PcRuntime* r,PcContext* c,void*) { return query(r,c,true); }
}
extern "C" PcStatus pc_register_time_imports(PcRuntime* r) {
    const PcGuestFunction handlers[] = {ticks,multimedia_ticks,counter,frequency};
    for (uint32_t i=0;i<4;++i) {
        const auto status = pc_register_import(r,i+1,handlers[i],nullptr);
        if (status != PC_OK) return status;
    }
    return PC_OK;
}
