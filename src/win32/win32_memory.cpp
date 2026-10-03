#include "win32_shims.h"
#include <cstdlib>

namespace pcrecomp::win32 {

void* shim_VirtualAlloc(void* lpAddress, size_t dwSize, uint32_t flAllocationType, uint32_t flProtect) {
    (void)flAllocationType;
    (void)flProtect;
    if (lpAddress) {
        return lpAddress;
    }
    return std::malloc(dwSize);
}

int32_t shim_VirtualProtect(void* lpAddress, size_t dwSize, uint32_t flNewProtect, uint32_t* lpflOldProtect) {
    (void)lpAddress;
    (void)dwSize;
    (void)flNewProtect;
    if (lpflOldProtect) {
        *lpflOldProtect = 0x40; // PAGE_EXECUTE_READWRITE
    }
    return 1; // Success
}

int32_t shim_VirtualFree(void* lpAddress, size_t dwSize, uint32_t dwFreeType) {
    (void)dwSize;
    (void)dwFreeType;
    if (lpAddress && (uintptr_t)lpAddress > 0x80000000ULL) {
        std::free(lpAddress);
    }
    return 1;
}

void* shim_HeapCreate(uint32_t flOptions, size_t dwInitialSize, size_t dwMaximumSize) {
    (void)flOptions;
    (void)dwInitialSize;
    (void)dwMaximumSize;
    return reinterpret_cast<void*>(0x00020000ULL);
}

int32_t shim_HeapDestroy(void* hHeap) {
    (void)hHeap;
    return 1;
}

size_t shim_HeapSize(void* hHeap, uint32_t dwFlags, const void* lpMem) {
    (void)hHeap;
    (void)dwFlags;
    (void)lpMem;
    return 512;
}

} // namespace pcrecomp::win32
