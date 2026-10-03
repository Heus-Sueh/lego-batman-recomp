"""Generation of runtime support translation units (CRT, WinMain, RenderWare shims)."""

from pathlib import Path
from .common import HEADER_COMMENT, write_if_changed


def generate_crt_cpp(out_dir: Path):
    content = HEADER_COMMENT + """
#include "recomp_funcs.h"
#include "../../thirdparty/pcrecomp/runtime/recomp32/recomp_types.h"
#include <iostream>

#define RECOMP_GENERATED_CODE

extern "C" {

// mainCRTStartup (0x008246EF) - Legacy entry point bridge to authentic WinMainCRTStartup (0x00824570)
void recomp_func_008246EF(void) {
    RECOMP_ENTER(0x008246EFu);
    std::cout << "[Recomp] Dispatching through authentic WinMainCRTStartup (0x00824570)...\\n";
    recomp_func_00824570();
    std::cout << "[Recomp] Authentic WinMainCRTStartup (0x00824570) finished with status " << g_eax << "\\n";
}

// _initterm (0x00824588) - CRT initializer iterator
void recomp_func_00824588(void) {
    RECOMP_ENTER(0x00824588u);
    g_eax = 0;
}

} // extern "C"
"""
    write_if_changed(out_dir / "gta_sa_crt.cpp", content)


def generate_winmain_cpp(out_dir: Path):
    content = HEADER_COMMENT + """
#include "recomp_funcs.h"
#include "../../thirdparty/pcrecomp/runtime/recomp32/recomp_types.h"
#include <iostream>
#include <chrono>

#define RECOMP_GENERATED_CODE

extern "C" {
int32_t g_max_frames = -1;
uint32_t g_frame_counter = 0;

// WinMain (0x00748DA0) - Legacy entry point bridge to authentic WinMain (0x00748710)
void recomp_func_00748DA0(void) {
    RECOMP_ENTER(0x00748DA0u);
    std::cout << "[Recomp] Entering authentic WinMain (0x00748710)...\\n";
    PUSH32(g_esp, 10);          // nCmdShow (SW_SHOWDEFAULT)
    PUSH32(g_esp, 0x00824000);  // lpCmdLine
    PUSH32(g_esp, 0);           // hPrevInstance
    PUSH32(g_esp, 0x00400000);  // hInstance
    recomp_func_00748710();
    std::cout << "[Recomp] Authentic WinMain (0x00748710) returned with status " << g_eax << "\\n";
}

// InitApp (0x00745080) - Legacy bridge to authentic InitApp (0x007468E0)
void recomp_func_00745080(void) {
    RECOMP_ENTER(0x00745080u);
    recomp_func_007468E0();
}

// MainWndProc (0x00747EB0) - Window procedure callback (stdcall, purge 16)
void recomp_func_00747EB0(void) {
    RECOMP_ENTER(0x00747EB0u);
    g_esp += 16;
    g_eax = 0;
}

// RsRWInitialise (0x00619C90) - RenderWare device initialization helper
void recomp_func_00619C90(void) {
    RECOMP_ENTER(0x00619C90u);
    std::cout << "[Recomp] Authentic RenderWare rsRWINIT helper (0x00619C90) executed.\\n";
    g_eax = 1;
}

// CTimer helpers for WinMain frame loop
void recomp_func_00561A80(void) {
    RECOMP_ENTER(0x00561A80u);
    static const auto s_start = std::chrono::steady_clock::now();
    static uint64_t s_sim_cycles = 0;
    if (g_max_frames > 0) {
        s_sim_cycles += 16000; // 16ms step in bounded frame test mode
        g_eax = static_cast<uint32_t>(s_sim_cycles);
    } else {
        auto now = std::chrono::steady_clock::now();
        uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(now - s_start).count();
        g_eax = static_cast<uint32_t>(us);
    }
}

void recomp_func_00561A40(void) {
    RECOMP_ENTER(0x00561A40u);
    g_eax = 1000; // 1000 cycles per ms (1 cycle = 1 us)
}

} // extern "C"
"""
    write_if_changed(out_dir / "gta_sa_winmain.cpp", content)


def generate_rw_cpp(out_dir: Path):
    content = HEADER_COMMENT + """
#include "recomp_funcs.h"
#include "../../thirdparty/pcrecomp/runtime/recomp32/recomp_types.h"
#include "../../src/rw/rw_core.h"
#include <iostream>

#define RECOMP_GENERATED_CODE

extern "C" {

// RsEventHandler (0x00619B70) - Legacy bridge to authentic RsEventHandler (0x00619B60)
void recomp_func_00619B70(void) {
    RECOMP_ENTER(0x00619B70u);
    uint32_t event = g_eax;
    uint32_t param = g_edx;
    PUSH32(g_esp, param);
    PUSH32(g_esp, event);
    recomp_func_00619B60();
    POP32_VAL(g_esp);
    POP32_VAL(g_esp);
}

// RwEngineInit (0x007F6F10)
void recomp_func_007F6F10(void) {
    RECOMP_ENTER(0x007F6F10u);
    std::cout << "[Recomp] Executed RwEngineInit (0x007F6F10)\\n";
    // Point RwEngineInstance global (0x00C97B24) to a non-null valid sentinel
    MEM32(0x00C97B24) = 0x00C97000;
    g_eax = 1;
}

// RwEngineOpen (0x007F6BE0)
void recomp_func_007F6BE0(void) {
    RECOMP_ENTER(0x007F6BE0u);
    std::cout << "[Recomp] Executed RwEngineOpen (0x007F6BE0)\\n";
    g_eax = 1;
}

// RwEngineStart (0x007F6D80)
void recomp_func_007F6D80(void) {
    RECOMP_ENTER(0x007F6D80u);
    std::cout << "[Recomp] Executed RwEngineStart (0x007F6D80)\\n";
    g_eax = 1;
}

// RwEngineStop (0x007F6D40)
void recomp_func_007F6D40(void) {
    RECOMP_ENTER(0x007F6D40u);
    g_eax = 1;
}

// RwEngineClose (0x007F6C90)
void recomp_func_007F6C90(void) {
    RECOMP_ENTER(0x007F6C90u);
    g_eax = 1;
}

// RwEngineTerm (0x007F6EC0)
void recomp_func_007F6EC0(void) {
    RECOMP_ENTER(0x007F6EC0u);
    MEM32(0x00C97B24) = 0;
    g_eax = 1;
}

// RwOsGetFileInterface (0x00801CF0)
void recomp_func_00801CF0(void) {
    RECOMP_ENTER(0x00801CF0u);
    std::cout << "[Recomp] Executed RwOsGetFileInterface (0x00801CF0)\\n";
    g_eax = 0x008E2DE0; // Return pointer to file interface
}

// RwCameraBeginUpdate (0x007EE130)
void recomp_func_007EE130(void) {
    RECOMP_ENTER(0x007EE130u);
    pcrecomp::rw::RwCameraBeginUpdate(nullptr);
    g_eax = 0x00C1703C; // Return Scene.camera
}

// RwCameraEndUpdate (0x007EE140)
void recomp_func_007EE140(void) {
    RECOMP_ENTER(0x007EE140u);
    pcrecomp::rw::RwCameraEndUpdate(nullptr);
    g_eax = 0x00C1703C;
}

// RwCameraClear (0x007EE1D0)
void recomp_func_007EE1D0(void) {
    RECOMP_ENTER(0x007EE1D0u);
    pcrecomp::rw::RwCameraClear(nullptr, nullptr, 3);
    g_eax = 0x00C1703C;
}

// RwRasterCreate (0x007FA330)
void recomp_func_007FA330(void) {
    RECOMP_ENTER(0x007FA330u);
    int32_t width = static_cast<int32_t>(MEM32(g_esp + 4));
    int32_t height = static_cast<int32_t>(MEM32(g_esp + 8));
    int32_t depth = static_cast<int32_t>(MEM32(g_esp + 12));
    int32_t flags = static_cast<int32_t>(MEM32(g_esp + 16));
    auto* r = pcrecomp::rw::RwRasterCreate(width, height, depth, flags);
    g_eax = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(r));
}

// RwRasterDestroy (0x007FA520)
void recomp_func_007FA520(void) {
    RECOMP_ENTER(0x007FA520u);
    auto* r = reinterpret_cast<pcrecomp::rw::RwRaster*>(static_cast<uintptr_t>(MEM32(g_esp + 4)));
    g_eax = pcrecomp::rw::RwRasterDestroy(r) ? 1 : 0;
}

// RwRasterLock (0x007FA140)
void recomp_func_007FA140(void) {
    RECOMP_ENTER(0x007FA140u);
    auto* r = reinterpret_cast<pcrecomp::rw::RwRaster*>(static_cast<uintptr_t>(MEM32(g_esp + 4)));
    uint8_t level = static_cast<uint8_t>(MEM32(g_esp + 8));
    int32_t lockMode = static_cast<int32_t>(MEM32(g_esp + 12));
    auto* pixels = pcrecomp::rw::RwRasterLock(r, level, lockMode);
    g_eax = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pixels));
}

// RwRasterUnlock (0x007FA240)
void recomp_func_007FA240(void) {
    RECOMP_ENTER(0x007FA240u);
    auto* r = reinterpret_cast<pcrecomp::rw::RwRaster*>(static_cast<uintptr_t>(MEM32(g_esp + 4)));
    auto* res = pcrecomp::rw::RwRasterUnlock(r);
    g_eax = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(res));
}

// RwRasterShowRaster (0x007FA910)
void recomp_func_007FA910(void) {
    RECOMP_ENTER(0x007FA910u);
    auto* r = reinterpret_cast<pcrecomp::rw::RwRaster*>(static_cast<uintptr_t>(MEM32(g_esp + 4)));
    void* dev = reinterpret_cast<void*>(static_cast<uintptr_t>(MEM32(g_esp + 8)));
    uint32_t flags = MEM32(g_esp + 12);
    auto* res = pcrecomp::rw::RwRasterShowRaster(r, dev, flags);
    g_eax = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(res));
}

// RwTextureCreate (0x007F37C0)
void recomp_func_007F37C0(void) {
    RECOMP_ENTER(0x007F37C0u);
    auto* r = reinterpret_cast<pcrecomp::rw::RwRaster*>(static_cast<uintptr_t>(MEM32(g_esp + 4)));
    auto* t = pcrecomp::rw::RwTextureCreate(r);
    g_eax = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(t));
}

// RwTextureDestroy (0x007F3820)
void recomp_func_007F3820(void) {
    RECOMP_ENTER(0x007F3820u);
    auto* t = reinterpret_cast<pcrecomp::rw::RwTexture*>(static_cast<uintptr_t>(MEM32(g_esp + 4)));
    g_eax = pcrecomp::rw::RwTextureDestroy(t) ? 1 : 0;
}

// RwIm2DRenderPrimitive (0x007EE4B0)
void recomp_func_007EE4B0(void) {
    RECOMP_ENTER(0x007EE4B0u);
    auto prim = static_cast<pcrecomp::rw::RwPrimitiveType>(MEM32(g_esp + 4));
    auto* verts = reinterpret_cast<pcrecomp::rw::RwIm2DVertex*>(static_cast<uintptr_t>(MEM32(g_esp + 8)));
    int32_t numVerts = static_cast<int32_t>(MEM32(g_esp + 12));
    g_eax = pcrecomp::rw::RwIm2DRenderPrimitive(prim, verts, numVerts) ? 1 : 0;
}

// RwIm2DRenderIndexedPrimitive (0x007EE5A0)
void recomp_func_007EE5A0(void) {
    RECOMP_ENTER(0x007EE5A0u);
    auto prim = static_cast<pcrecomp::rw::RwPrimitiveType>(MEM32(g_esp + 4));
    auto* verts = reinterpret_cast<pcrecomp::rw::RwIm2DVertex*>(static_cast<uintptr_t>(MEM32(g_esp + 8)));
    int32_t numVerts = static_cast<int32_t>(MEM32(g_esp + 12));
    auto* indices = reinterpret_cast<uint16_t*>(static_cast<uintptr_t>(MEM32(g_esp + 16)));
    int32_t numIndices = static_cast<int32_t>(MEM32(g_esp + 20));
    g_eax = pcrecomp::rw::RwIm2DRenderIndexedPrimitive(prim, verts, numVerts, indices, numIndices) ? 1 : 0;
}

} // extern "C"
"""
    write_if_changed(out_dir / "gta_sa_rw.cpp", content)
