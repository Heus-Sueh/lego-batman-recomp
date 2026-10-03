#include <pcmodern/memory.h>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <sys/mman.h>
#endif
namespace pcmodern {
void* allocate_memory(size_t size) {
    if (!size) return nullptr;
#if defined(_WIN32)
    return VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
#else
    int flags=MAP_PRIVATE|MAP_ANONYMOUS;
#ifdef MAP_NORESERVE
    flags|=MAP_NORESERVE;
#endif
    void* memory=mmap(nullptr,size,PROT_READ|PROT_WRITE,flags,-1,0);
    return memory==MAP_FAILED ? nullptr : memory;
#endif
}
void release_memory(void* address,size_t size) {
    if (!address) return;
#if defined(_WIN32)
    (void)size;
    VirtualFree(address,0,MEM_RELEASE);
#else
    munmap(address,size);
#endif
}
}
