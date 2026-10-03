#include "win32_shims.h"
#include "../platform/platform.h"
#include "../runtime/guest_memory.h"
#include "../runtime/diagnostics.h"
#include <iostream>

extern "C" {
extern int32_t g_max_frames;
extern uint32_t g_frame_counter;
}

namespace pcrecomp::win32 {

void* shim_CreateWindowExA(uint32_t dwExStyle, const char* lpClassName, const char* lpWindowName,
                          uint32_t dwStyle, int32_t X, int32_t Y, int32_t nWidth, int32_t nHeight,
                          void* hWndParent, void* hMenu, void* hInstance, void* lpParam) {
    (void)dwExStyle;
    (void)lpClassName;
    (void)dwStyle;
    (void)X;
    (void)Y;
    (void)hWndParent;
    (void)hMenu;
    (void)hInstance;
    (void)lpParam;

    std::cout << "[Win32] CreateWindowExA: \"" << (lpWindowName ? lpWindowName : "PC Recomp")
              << "\" (" << nWidth << "x" << nHeight << ")\n";
    void* handle = platform::Platform::instance().native_window_handle();
    if (!handle) {
        handle = reinterpret_cast<void*>(0x12340001); // Fallback dummy handle for headless mode
    }

#ifdef RECOMP_LEGACY_GAME_HOOKS
    constexpr uint32_t rsPlatformSpec_va = 0x00C17060u;
    uint32_t hwnd_val = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(handle));
    auto& gm = runtime::GuestMemory::instance();
    if (gm.is_initialized()) {
        gm.write32(0x00C17054, rsPlatformSpec_va);       // RsGlobal.ps = &winPS
        gm.write32(rsPlatformSpec_va + 0, hwnd_val);      // ps->window = hwnd
        gm.write32(rsPlatformSpec_va + 4, 0x00400000u);   // ps->instance = hInstance
        gm.write32(0x008D621C, 1);                        // g_bWindowActive = 1
    }
#endif
    return handle;
}

int32_t shim_ShowWindow(void* hWnd, int32_t nCmdShow) {
    (void)hWnd;
    (void)nCmdShow;
    return 1;
}

int32_t shim_DestroyWindow(void* hWnd) {
    (void)hWnd;
#ifdef RECOMP_LEGACY_GAME_HOOKS
    auto& gm = runtime::GuestMemory::instance();
    if (gm.is_initialized()) {
        uint32_t ps = gm.read32(0x00C17054);
        if (ps) {
            gm.write32(ps, 0); // ps->window = NULL
        }
    }
#endif
    return 1;
}

int32_t shim_AdjustWindowRectEx(RECT* lpRect, uint32_t dwStyle, int32_t bMenu, uint32_t dwExStyle) {
    (void)lpRect;
    (void)dwStyle;
    (void)bMenu;
    (void)dwExStyle;
    return 1;
}

int32_t shim_AdjustWindowRect(RECT* lpRect, uint32_t dwStyle, int32_t bMenu) {
    (void)lpRect;
    (void)dwStyle;
    (void)bMenu;
    return 1;
}

int32_t shim_SetWindowTextA(void* hWnd, const char* lpString) {
    (void)hWnd;
    (void)lpString;
    return 1;
}

int32_t shim_PeekMessageA(void* lpMsg, void* hWnd, uint32_t wMsgFilterMin, uint32_t wMsgFilterMax, uint32_t wRemoveMsg) {
    (void)hWnd;
    (void)wMsgFilterMin;
    (void)wMsgFilterMax;
    (void)wRemoveMsg;

    auto& platform = platform::Platform::instance();
    bool running = platform.poll_events();
    ++g_frame_counter;

    if (!running || (g_max_frames > 0 && static_cast<int32_t>(g_frame_counter) >= g_max_frames)) {
        std::cout << "[shim_PeekMessageA] Quit triggered! running=" << running
                  << " g_frame_counter=" << g_frame_counter << " g_max_frames=" << g_max_frames << "\n";
#ifdef RECOMP_LEGACY_GAME_HOOKS
        runtime::GuestMemory::instance().write32(0x00C17050, 1); // Legacy
        runtime::GuestMemory::instance().write32(0x00AD21BC, 1);
#endif
        if (lpMsg) {
            uint32_t* msg_words = reinterpret_cast<uint32_t*>(lpMsg);
            msg_words[0] = 0;      // hwnd
            msg_words[1] = 0x0012; // WM_QUIT
            msg_words[2] = 0;      // wParam
            msg_words[3] = 0;      // lParam
        }
        throw runtime::FrameLimitReached{};
    }
    return 0; // No pending messages
}

int32_t shim_DefWindowProcA(void* hWnd, uint32_t Msg, uint64_t wParam, int64_t lParam) {
    (void)hWnd;
    (void)Msg;
    (void)wParam;
    (void)lParam;
    return 0;
}

void* shim_LoadIconA(void* hInstance, const char* lpIconName) {
    (void)hInstance;
    (void)lpIconName;
    return reinterpret_cast<void*>(0x1001);
}

void* shim_LoadCursorA(void* hInstance, const char* lpCursorName) {
    (void)hInstance;
    (void)lpCursorName;
    return reinterpret_cast<void*>(0x2001);
}

uint16_t shim_RegisterClassA(const void* lpWndClass) {
    (void)lpWndClass;
    return 0xC001;
}

uint16_t shim_RegisterClassExA(const void* lpwcx) {
    (void)lpwcx;
    std::cout << "[Win32] RegisterClassExA: registered TTalesWindow class\n";
    return 0xC001;
}

int32_t shim_SendMessageA(void* hWnd, uint32_t Msg, uint64_t wParam, int64_t lParam) {
    (void)hWnd;
    (void)Msg;
    (void)wParam;
    (void)lParam;
    return 0;
}

void* shim_FindWindowA(const char* lpClassName, const char* lpWindowName) {
    (void)lpClassName;
    (void)lpWindowName;
    return nullptr;
}

int32_t shim_SetForegroundWindow(void* hWnd) {
    (void)hWnd;
    return 1;
}

int32_t shim_GetWindowPlacement(void* hWnd, void* lpwndpl) {
    (void)hWnd;
    if (lpwndpl) {
        // Set length = sizeof(WINDOWPLACEMENT) = 44, showCmd = SW_SHOWNORMAL (1) at offset 8
        *reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(lpwndpl) + 0) = 44;
        *reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(lpwndpl) + 8) = 1;
    }
    return 1;
}

int32_t shim_SystemParametersInfoA(uint32_t uiAction, uint32_t uiParam, void* pvParam, uint32_t fWinIni) {
    (void)uiAction;
    (void)uiParam;
    (void)pvParam;
    (void)fWinIni;
    return 1;
}

int32_t shim_ShowCursor(int32_t bShow) {
    return bShow ? 1 : 0;
}

int32_t shim_SetWindowLongA(void* hWnd, int32_t nIndex, int32_t dwNewLong) {
    (void)hWnd;
    (void)nIndex;
    (void)dwNewLong;
    return 0;
}

int32_t shim_UpdateWindow(void* hWnd) {
    (void)hWnd;
    return 1;
}

int32_t shim_SetWindowPos(void* hWnd, void* hWndInsertAfter, int32_t X, int32_t Y, int32_t cx, int32_t cy, uint32_t uFlags) {
    (void)hWnd;
    (void)hWndInsertAfter;
    (void)X;
    (void)Y;
    (void)cx;
    (void)cy;
    (void)uFlags;
    return 1;
}

void shim_PostQuitMessage(int32_t nExitCode) {
    (void)nExitCode;
    platform::Platform::instance().request_exit();
}

} // namespace pcrecomp::win32
