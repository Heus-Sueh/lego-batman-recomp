#pragma once

#include <cstdint>

#if defined(_WIN32)
    #define NOMINMAX
    #include <windows.h>
#else

namespace pcrecomp::win32 {

using DWORD = uint32_t;
using WORD = uint16_t;
using BYTE = uint8_t;
using BOOL = int32_t;
using LONG = int32_t;
using ULONG = uint32_t;
using LPVOID = void*;
using LPCSTR = const char*;
using LPSTR = char*;
using HANDLE = void*;
using HINSTANCE = void*;
using HMODULE = void*;
using HWND = void*;
using HICON = void*;
using HCURSOR = void*;
using HBRUSH = void*;
using HMENU = void*;
using HDC = void*;
using LRESULT = int64_t;
using WPARAM = uint64_t;
using LPARAM = int64_t;

constexpr BOOL TRUE = 1;
constexpr BOOL FALSE = 0;

struct RECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

struct POINT {
    LONG x;
    LONG y;
};

} // namespace pcrecomp::win32

#endif
