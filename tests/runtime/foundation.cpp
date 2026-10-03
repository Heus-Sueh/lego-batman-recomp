#include <pcrecomp/memory.h>
#include <pcrecomp/dispatch.h>
#include <pcrecomp/imports.h>
#include <pcrecomp/callbacks.h>
#include <pcrecomp/modules.h>
#include <pcrecomp/win32_time.h>
#include <pcmodern/time.h>
#include <pcmodern/memory.h>
#include <pcmodern/filesystem.h>
#include <fstream>
#include <win32compat/time.h>
#include <array>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdexcept>
extern "C" int c_generated_call(PcRuntime*, PcContext*);
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"check failed at %d: %s\n",__LINE__,#x); std::exit(1); } } while(false)
static PcFault fault{};
static unsigned faults=0;
static void diagnostic(void*,const PcFault* f) { fault=*f; ++faults; }
static PcStatus guest(PcRuntime*,PcContext* c,void* data) { c->eax+=*static_cast<uint32_t*>(data); return PC_OK; }
int main() {
    void* allocated=pcmodern::allocate_memory(4096);
    CHECK(allocated && static_cast<uint8_t*>(allocated)[4095]==0);
    pcmodern::release_memory(allocated,4096);
    CHECK(!pcmodern::allocate_memory(0));
    const auto root=std::filesystem::temp_directory_path()/std::filesystem::path("pcrecomp-runtime-test-"+std::to_string(pcmodern::time_ns()));
    CHECK(std::filesystem::create_directory(root));
    std::filesystem::create_directory(root/"Data");
    { std::ofstream file(root/"Data"/"Example.txt"); file << "synthetic fixture"; }
    auto& fs=pcmodern::VirtualFileSystem::instance();
    fs.set_root(root);
    const auto resolved=fs.resolve("data\\example.TXT");
    CHECK(resolved && std::filesystem::equivalent(*resolved,root/"Data"/"Example.txt"));
    CHECK(!fs.resolve("missing.bin"));
    fs.set_root(root/"Data");
    CHECK(!fs.resolve("data\\example.TXT"));
    std::filesystem::remove(root/"Data"/"Example.txt");
    std::filesystem::remove(root/"Data");
    std::filesystem::remove(root);
    PcRuntimeConfig config{}; config.diagnostic=diagnostic;
    PcRuntime* r=pc_runtime_create(&config); CHECK(r);
    std::array<uint8_t,128> memory{};
    CHECK(pc_bind_memory(r,memory.data(),memory.size())==PC_OK);
    CHECK(pc_write8(r,0,0xab)==PC_OK);
    CHECK(pc_write16(r,1,0x1234)==PC_OK);
    CHECK(pc_write32(r,3,0x12345678)==PC_OK);
    uint8_t b=0; uint16_t w=0; uint32_t d=0;
    CHECK(pc_read8(r,0,&b)==PC_OK && b==0xab);
    CHECK(pc_read16(r,1,&w)==PC_OK && w==0x1234);
    CHECK(pc_read32(r,3,&d)==PC_OK && d==0x12345678);
    CHECK(memory[1]==0x34 && memory[6]==0x12);
    void* translated=nullptr;
    CHECK(pc_translate(r,3,4,&translated)==PC_OK && translated==memory.data()+3);
    auto before=memory;
    CHECK(pc_write32(r,126,1)==PC_MEMORY_BOUNDS && memory==before);
    CHECK(pc_translate(r,UINT32_MAX,SIZE_MAX,&translated)==PC_MEMORY_BOUNDS && !translated);
    CHECK(pc_read32(r,128,&d)==PC_MEMORY_BOUNDS && d==0x12345678);
    CHECK(pc_read32(r,0,nullptr)==PC_INVALID_ARGUMENT);
    uint32_t increment=7;
    CHECK(pc_register_guest_function(r,0x100,guest,&increment)==PC_OK);
    CHECK(pc_register_guest_function(r,0x100,guest,&increment)==PC_DUPLICATE);
    PcContext c{}; c.esp=32;
    CHECK(pc_call_guest(r,0x100,&c,0x80)==PC_OK && c.eax==7);
    CHECK(pc_call_indirect(r,0x100,&c,0x80)==PC_OK && c.eax==14);
    CHECK(pc_invoke_guest_callback(r,0x100,&c,0x80)==PC_OK && c.eax==21);
    auto old=c;
    CHECK(pc_call_indirect(r,0x101,&c,0x80)==PC_UNKNOWN_TARGET);
    CHECK(fault.caller==0x80 && fault.target==0x101 && fault.context.esp==32 && fault.context.eax==21);
    CHECK(std::strcmp(fault.category,"icall")==0 && std::memcmp(&c,&old,sizeof(c))==0);
    CHECK(pc_call_guest(r,0x101,&c,0)==PC_UNKNOWN_TARGET);
    CHECK(pc_invoke_guest_callback(r,0x101,&c,0)==PC_UNKNOWN_TARGET);
    CHECK(pc_call_import(r,999,&c,0)==PC_UNKNOWN_TARGET);
    CHECK(pc_register_guest_function(r,0x102,[](PcRuntime*,PcContext*,void*) -> PcStatus { throw std::runtime_error("negative control"); },nullptr)==PC_OK);
    CHECK(pc_call_guest(r,0x102,&c,0)==PC_INTERNAL_ERROR);
    PcGuestModule module{"synthetic.exe",0x100,0x100,0x110}, found{};
    CHECK(pc_register_module(r,&module)==PC_OK);
    CHECK(pc_register_module(r,&module)==PC_DUPLICATE);
    CHECK(pc_find_module(r,0x110,&found)==PC_OK && found.entry_point==0x110);
    CHECK(pc_find_module(r,0x200,&found)==PC_UNKNOWN_TARGET);
    module.image_base=UINT32_MAX; module.image_size=2;
    CHECK(pc_register_module(r,&module)==PC_INVALID_ARGUMENT);
    uint64_t now=1'000'000'000;
    pcmodern::HostCallbacks backend{};
    backend.user=&now;
    backend.time_ns=[](void* u) { return *static_cast<uint64_t*>(u); };
    backend.sleep_ms=[](void* u,uint32_t ms) { *static_cast<uint64_t*>(u)+=uint64_t(ms)*1'000'000; };
    pcmodern::set_host_callbacks(backend);
    CHECK(pc_register_time_imports(r)==PC_OK);
    for (uint32_t ms : {1000u,2000u,3000u}) {
        now=uint64_t(ms)*1'000'000; c.esp=32;
        CHECK(c_generated_call(r,&c)==PC_OK && c.eax==ms && c.esp==36);
        CHECK(win32compat::time_get_time()==ms);
    }
    win32compat::sleep(10); CHECK(now==3'010'000'000);
    now=(uint64_t(UINT32_MAX)+2)*1'000'000;
    CHECK(win32compat::get_tick_count()==1);
    now=3'000'000'000; c.esp=32;
    CHECK(pc_write32(r,36,80)==PC_OK);
    CHECK(pc_call_import(r,PC_TIME_QUERY_COUNTER,&c,0x120)==PC_OK && c.eax==1 && c.esp==40);
    CHECK(pc_read32(r,80,&d)==PC_OK && d==3'000'000'000u);
    c.esp=32;
    CHECK(pc_call_import(r,PC_TIME_QUERY_FREQUENCY,&c,0x120)==PC_OK);
    CHECK(pc_read32(r,80,&d)==PC_OK && d==1'000'000'000u);
    c.esp=32; CHECK(pc_write32(r,36,126)==PC_OK);
    CHECK(pc_call_import(r,PC_TIME_QUERY_COUNTER,&c,0)==PC_MEMORY_BOUNDS && c.esp==32);
    CHECK(pc_write32(r,36,0)==PC_OK);
    CHECK(pc_call_import(r,PC_TIME_QUERY_COUNTER,&c,0)==PC_OK && c.eax==0 && c.esp==40);
    CHECK(faults>=7);
    CHECK(pc_bind_memory(r,nullptr,0)==PC_OK);
    CHECK(pc_read8(r,0,&b)==PC_MEMORY_BOUNDS);
    pcmodern::set_host_callbacks({});
    pc_runtime_destroy(r);
    std::puts("Runtime foundation: memory, dispatch, ICALL, callbacks, modules, C ABI and clock imports passed");
}
