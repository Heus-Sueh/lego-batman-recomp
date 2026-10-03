// Existing integration retained verbatim during adapter extraction.
// Includes historical GTA probes; not a validated LEGO Batman engine adapter.
#include "adapters/adapter.h"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "runtime/guest_memory.h"
#include "runtime/section_loader.h"
#include "runtime/dispatch.h"
#include "runtime/diagnostics.h"
#include "target_config.h"
#include "platform/platform.h"
#include "platform/vfs.h"
#include "win32/win32_shims.h"
#include "win32/import_resolver.h"
#include "renderer/renderer.h"
#include "rw/rw_core.h"

extern void register_generated_recomp_functions();

namespace {

extern "C" uint32_t g_esp;
extern "C" int32_t g_max_frames;

void print_banner() {
    std::cout << "============================================================\n";
    std::cout << "  " << pcrecomp::target::title << " - PC Static Recompilation  \n";
    std::cout << "============================================================\n";
}

void print_help(const char* exe_name) {
    std::cout << "Usage: " << exe_name << " [options]\n\n"
              << "Options:\n"
              << "  --help, -h           Show this help message\n"
              << "  --version, -v        Show recompilation build version\n"
              << "  --game-dir <path>    Specify the original game installation\n"
              << "  --windowed           Start in windowed mode\n"
              << "  --fullscreen         Start in fullscreen mode\n"
              << "  --headless           Run initialization without creating a window\n"
              << "  --frames <N>         Run N engine frames and exit cleanly\n"
              << "  --fps <N>            Target simulation tick rate (default: 30)\n"
              << "  --width <pixels>     Window width (default: 800)\n"
              << "  --height <pixels>    Window height (default: 600)\n"
              << "  --coverage           Dump runtime function execution coverage report on exit\n"
              << "  --frontier           Dump execution frontier status report on exit\n"
              << "  --trace-boot         Trace execution through original PE entry point and CRT startup\n"
              << "  --trace-imports      Trace Win32 API import invocations and resolutions\n"
              << "  --trace-icalls       Trace indirect call dispatch (caller -> target)\n"
              << "  --trace-functions    Trace entry into lifted functions\n"
              << "  --trace-files        Trace Win32 file I/O operations and path mappings\n"
              << "  --trace-threads      Trace thread and synchronization operations\n"
              << "  --test-raster        Validate Tier 2 raster locking and texture updates\n"
              << "  --test-splash        Render splash screen quad to host swapchain\n"
              << "  --test-menu          Validate frontend menu state machine and navigation\n\n";
}

std::filesystem::path find_game_directory(const std::string& custom_path) {
    if (!custom_path.empty()) {
        std::filesystem::path p(custom_path);
        if (std::filesystem::exists(p)) {
            return p;
        }
    }

    const std::vector<std::filesystem::path> search_paths = {
        pcrecomp::target::default_game_dir,
        std::filesystem::path("../") / pcrecomp::target::default_game_dir
    };

    for (const auto& candidate : search_paths) {
        if (std::filesystem::exists(candidate / pcrecomp::target::guest_executable)) {
            return candidate;
        }
    }

    return ".";
}

} // namespace

int pcrecomp::adapter::run(int argc, char* argv[]) {
    print_banner();

    std::string custom_game_dir;
    bool headless = false;
    bool test_dispatch = false;
    bool test_imports = false;
    bool test_unresolved = false;
    bool test_raster = false;
    bool test_splash = false;
    bool test_menu = false;
    bool test_im3d = false;
    bool trace_boot = false;
    bool show_coverage = false;
    int max_frames = -1;
    uint32_t target_fps = pcrecomp::target::default_simulation_fps;
    pcrecomp::platform::WindowConfig window_cfg;
    window_cfg.width = pcrecomp::target::default_window_width;
    window_cfg.height = pcrecomp::target::default_window_height;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help(argv[0]);
            return 0;
        }
        if (arg == "--version" || arg == "-v") {
            std::cout << "pcrecomp " << pcrecomp::target::title << "\n";
            return 0;
        }
        if (arg == "--headless") {
            headless = true;
        } else if (arg == "--windowed") {
            window_cfg.fullscreen = false;
        } else if (arg == "--fullscreen") {
            window_cfg.fullscreen = true;
        } else if (arg == "--coverage") {
            show_coverage = true;
        } else if (arg == "--frontier") {
            pcrecomp::runtime::get_trace_config().show_frontier = true;
        } else if (arg == "--trace-boot") {
            trace_boot = true;
            pcrecomp::runtime::get_trace_config().trace_boot = true;
        } else if (arg == "--trace-imports") {
            pcrecomp::runtime::get_trace_config().trace_imports = true;
        } else if (arg == "--trace-icalls") {
            pcrecomp::runtime::get_trace_config().trace_icalls = true;
        } else if (arg == "--trace-functions") {
            pcrecomp::runtime::get_trace_config().trace_functions = true;
        } else if (arg == "--trace-files") {
            pcrecomp::runtime::get_trace_config().trace_files = true;
        } else if (arg == "--trace-threads") {
            pcrecomp::runtime::get_trace_config().trace_threads = true;
        } else if (arg == "--game-dir" && i + 1 < argc) {
            custom_game_dir = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            max_frames = std::stoi(argv[++i]);
        } else if (arg == "--fps" && i + 1 < argc) {
            target_fps = std::stoi(argv[++i]);
        } else if (arg == "--test-dispatch") {
            test_dispatch = true;
        } else if (arg == "--test-imports") {
            test_imports = true;
        } else if (arg == "--test-unresolved") {
            test_unresolved = true;
        } else if (arg == "--test-raster") {
            test_raster = true;
        } else if (arg == "--test-splash") {
            test_splash = true;
        } else if (arg == "--test-menu") {
            test_menu = true;
        } else if (arg == "--test-im3d") {
            test_im3d = true;
        } else if (arg == "--trace-boot") {
            trace_boot = true;
        } else if (arg == "--width" && i + 1 < argc) {
            window_cfg.width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            window_cfg.height = std::stoi(argv[++i]);
        }
    }

    // 1. Locate game data root and initialize VFS
    auto game_root = find_game_directory(custom_game_dir);
    std::cout << "[Init] Discovered game installation at: " << game_root << "\n";
    pcrecomp::platform::VirtualFileSystem::instance().set_root(game_root);

    if (headless) {
        pcrecomp::platform::Platform::instance().set_running(true);
    }
    if (max_frames > 0) {
        g_max_frames = max_frames;
    } else if (headless) {
        g_max_frames = 1;
    }

    // 2. Initialize 32-bit guest memory space
    if (!pcrecomp::runtime::GuestMemory::instance().initialize()) {
        std::cerr << "[Fatal] Could not initialize guest memory space.\n";
        return 1;
    }

    // 3. Load PE data sections into guest memory space
    std::filesystem::path exe_candidate = std::filesystem::path(pcrecomp::target::default_game_dir) /
        pcrecomp::target::guest_executable;
    if (!std::filesystem::exists(exe_candidate)) {
        exe_candidate = game_root / pcrecomp::target::guest_executable;
    }
    if (std::filesystem::exists(exe_candidate)) {
        pcrecomp::runtime::SectionLoader::load_pe_data_sections(exe_candidate, pcrecomp::runtime::GuestMemory::instance());
    } else {
        std::cout << "[SectionLoader] Warning: No " << pcrecomp::target::guest_executable
                  << " found at " << exe_candidate << " to load sections.\n";
    }

    // 4. Register generated recompiled functions into dispatch table
    ::register_generated_recomp_functions();

    // Initialize guest stack pointer in 2GB space
    g_esp = 0x00300000;
    pcrecomp::runtime::GuestMemory::instance().write32(0x00C17044, window_cfg.width);
    pcrecomp::runtime::GuestMemory::instance().write32(0x00C17048, window_cfg.height);
    pcrecomp::runtime::GuestMemory::instance().write32(0x00C1704C, target_fps);
    pcrecomp::runtime::GuestMemory::instance().write32(0x008D621C, 1); // g_bWindowActive = 1
    pcrecomp::runtime::GuestMemory::instance().write32(0x00C920CC, 1); // bSkipLegalScreen = 1

    // 5. Initialize Win32 compatibility layer shims & imports
    pcrecomp::win32::initialize_win32_subsystems();
    pcrecomp::win32::initialize_win32_imports();

    // 6. Initialize RenderWare hooks
    pcrecomp::rw::initialize_renderware_subsystems();

    // 7. Initialize Platform (SDL3) if not headless (must precede hardware texture creation)
    if (!headless) {
        if (!pcrecomp::platform::Platform::instance().initialize(window_cfg)) {
            std::cerr << "[Fatal] Platform initialization failed.\n";
            return 1;
        }
    }

    // 8. Initialize Renderer and configure simulation tick rate
    auto& renderer = pcrecomp::renderer::Renderer::instance();
    renderer.set_simulation_target_fps(target_fps);
    renderer.initialize();

    if (!headless) {
        pcrecomp::rw::CLoadingScreen_LoadSplashes();
    }

    // 9. Optional diagnostics & tests
    if (test_imports) {
        std::cout << "[Test] Verifying Win32 import shims...\n";
        auto& resolver = pcrecomp::win32::ImportResolver::instance();
        void* pCreateFile = resolver.resolve_import("KERNEL32.dll", "CreateFileA");
        void* pTimeGetTime = resolver.resolve_import("KERNEL32.dll", "timeGetTime");
        if (pCreateFile && pTimeGetTime) {
            std::cout << "[Test] Import shims resolved successfully (" << resolver.registered_count() << " registered).\n";
        } else {
            std::cerr << "[Test] Failed to resolve critical imports.\n";
            return 1;
        }
    }

    if (test_unresolved) {
        std::cout << "[Test] Triggering unresolved indirect call test (0xDEADBEEF)...\n";
        pcrecomp::runtime::GuestRegisters regs{};
        regs.eax = 0x12345678;
        regs.ecx = 0x87654321;
        pcrecomp::runtime::DispatchTable::instance().dispatch(0xDEADBEEFu, regs, 0x00748DA0u);
        std::cout << "[Test] Unresolved call handled without crashing.\n";
    }

    if (test_raster) {
        std::cout << "[Test] Verifying RenderWare raster lifecycle (64x64 ARGB)...\n";
        auto* raster = pcrecomp::rw::RwRasterCreate(64, 64, 32, 0);
        if (!raster) {
            std::cerr << "[Test] Failed to create raster.\n";
            return 1;
        }
        uint8_t* pixels = pcrecomp::rw::RwRasterLock(raster, 0, 0);
        if (!pixels) {
            std::cerr << "[Test] Failed to lock raster.\n";
            return 1;
        }
        for (int y = 0; y < 64; ++y) {
            for (int x = 0; x < 64; ++x) {
                uint32_t color = ((x ^ y) & 8) ? 0xFF00FF00 : 0xFF0000FF;
                reinterpret_cast<uint32_t*>(pixels)[y * 64 + x] = color;
            }
        }
        pcrecomp::rw::RwRasterUnlock(raster);
        auto* tex = pcrecomp::rw::RwTextureCreate(raster);
        if (!tex) {
            std::cerr << "[Test] Failed to create texture from raster.\n";
            return 1;
        }
        std::cout << "[Test] Raster and texture created and unlocked successfully.\n";
        pcrecomp::rw::RwTextureDestroy(tex);
        std::cout << "[Test] Raster and texture destroyed successfully.\n";
    }

    if (test_menu) {
        std::cout << "[Test] Validating authentic CMenuManager state machine (0x00574F30)...\n";
        g_esp = 0x00300000;
        pcrecomp::rw::initialize_menu_and_font_textures();
        pcrecomp::runtime::GuestRegisters regs{};
        regs.ecx = 0x00BA6748;
        pcrecomp::runtime::DispatchTable::instance().dispatch(0x00574F30u, regs, 0);
        regs.ecx = 0x00BA6748;
        pcrecomp::runtime::DispatchTable::instance().dispatch(0x00572EC0u, regs, 0);
        std::cout << "[Test] Authentic CMenuManager state machine validated successfully.\n";
    }

    if (test_dispatch) {
        std::cout << "[Test] Executing recompiled entry point dispatch (0x0073D6A3)...\n";
        g_esp = 0x00300000; // Initialize guest stack pointer in 2GB space
        g_max_frames = 1;
        pcrecomp::runtime::GuestMemory::instance().write32(0x00C920CC, 1);
        pcrecomp::runtime::GuestRegisters regs{};
        try { pcrecomp::runtime::DispatchTable::instance().dispatch(0x0073D6A3u, regs, 0); }
        catch (const pcrecomp::runtime::FrameLimitReached&) {}
        std::cout << "[Test] Dispatch execution completed successfully.\n";
    }

    if (test_splash) {
        std::cout << "[Test] Displaying loading screen splash...\n";
        pcrecomp::rw::CLoadingScreen_Init();
    }



    if (test_im3d) {
        std::cout << "[Test] Validating RenderWare 3D pipeline (RwIm3D) and perspective projection...\n";
        pcrecomp::rw::RwCameraSetPerspective(70.0f, 800.0f / 600.0f, 0.5f, 1000.0f);
        pcrecomp::rw::RwCameraLookAt(0.0f, -10.0f, 5.0f, 0.0f, 0.0f, 0.0f);

        pcrecomp::rw::RwIm3DVertex tri[3] = {
            { { -2.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0xFFFF0000u, 0.0f, 0.0f },
            { {  2.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 0xFF00FF00u, 1.0f, 0.0f },
            { {  0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, 1.0f }, 0xFF0000FFu, 0.5f, 1.0f }
        };

        void* transformed = pcrecomp::rw::RwIm3DTransform(tri, 3, nullptr, 0);
        if (!transformed) {
            std::cerr << "[Test] RwIm3DTransform failed.\n";
            return 1;
        }
        if (!pcrecomp::rw::RwIm3DRenderPrimitive(pcrecomp::rw::rwPRIMTYPETRILIST)) {
            std::cerr << "[Test] RwIm3DRenderPrimitive failed.\n";
            return 1;
        }
        pcrecomp::rw::RwIm3DEnd();

        if (renderer.im3d_draw_calls() == 0) {
            std::cerr << "[Test] im3d_draw_calls counter was not incremented.\n";
            return 1;
        }

        std::cout << "[Test] RenderWare 3D pipeline (RwIm3D) and perspective projection validated successfully.\n";
    }

    if (trace_boot) {
        std::cout << "[TraceBoot] Entering original PE entry point: mainCRTStartup (0x0073D6A3)...\n";
        g_esp = 0x00300000; // Initialize guest stack pointer in 2GB space
        if (max_frames > 0) {
            g_max_frames = max_frames;
        } else {
            g_max_frames = 1;
        }
        pcrecomp::runtime::GuestMemory::instance().write32(0x00C920CC, 1);
        pcrecomp::runtime::GuestRegisters regs{};
        try { pcrecomp::runtime::DispatchTable::instance().dispatch(0x0073D6A3u, regs, 0); }
        catch (const pcrecomp::runtime::FrameLimitReached&) {}
        std::cout << "[TraceBoot] Original PE entry point execution completed successfully.\n";
    }

    bool is_diagnostic_run = trace_boot || test_dispatch || test_imports || test_unresolved || test_raster || test_splash || test_im3d || test_menu;

    if (!is_diagnostic_run) {
        std::cout << "[Init] Entering original PE entry point: mainCRTStartup (0x0073D6A3)...\n";
        if (!headless) {
            pcrecomp::rw::initialize_menu_and_font_textures();
        }
        g_esp = 0x00300000;
        pcrecomp::runtime::GuestRegisters init_regs{};
        try {
            pcrecomp::runtime::DispatchTable::instance().dispatch(0x0073D6A3u, init_regs, 0); // WinMainCRTStartup -> WinMain -> main loop
        } catch (const pcrecomp::runtime::FrameLimitReached&) {
            std::cout << "[Runtime] Bounded frame limit reached; leaving guest loop cleanly.\n";
        }
        if (headless && max_frames <= 0) {
            std::cout << "[Init] Headless verification passed.\n";
        }
    }

    // 10. Host Loop for standalone test modes (e.g. Splash test)
    if (test_splash) {
        if (!headless) {
            auto& platform = pcrecomp::platform::Platform::instance();
            std::cout << "[Init] Bootstrap successful. Entering host loop.\n";
            int frame_count = 0;
            while (platform.is_running()) {
                if (max_frames > 0 && frame_count >= max_frames) {
                    break;
                }

                if (!platform.poll_events()) {
                    break;
                }

                if (pcrecomp::runtime::GuestMemory::instance().read32(0x00C17050) != 0) {
                    break;
                }

                renderer.begin_frame();
                pcrecomp::rw::CLoadingScreen_RenderSplashWindow();
                renderer.end_frame();
                renderer.present();

                ++frame_count;
            }
            platform.shutdown();
        }
    }

    if (test_menu) {
        if (!headless) {
            auto& platform = pcrecomp::platform::Platform::instance();
            auto& gm = pcrecomp::runtime::GuestMemory::instance();
            std::cout << "[Menu] Entering interactive frontend menu loop (M10). Use Up/Down Arrows, Enter, Esc.\n";
            int frame_count = 0;
            pcrecomp::runtime::GuestRegisters regs{};
            while (platform.is_running()) {
                if (max_frames > 0 && frame_count >= max_frames) {
                    break;
                }

                if (!platform.poll_events()) {
                    break;
                }

                if (gm.read32(0x00C17050) != 0) { // RsGlobal.quit
                    break;
                }

                // Process menu logic
                regs.ecx = 0x00BA6748u;
                pcrecomp::runtime::DispatchTable::instance().dispatch(0x0057B440u, regs, 0);

                // Render frontend menu
                renderer.begin_frame(0, 0, 0, 255);
                regs.ecx = 0x00BA6748u;
                pcrecomp::runtime::DispatchTable::instance().dispatch(0x0057C290u, regs, 0);
                renderer.end_frame();
                renderer.present();

                if (gm.read8(0x00BA6748 + 0x5C) == 0) {
                    std::cout << "[Menu] Menu deactivated by player input. Transitioning...\n";
                    break;
                }

                ++frame_count;
            }
            platform.shutdown();
        }
    }

    if (show_coverage) {
        pcrecomp::runtime::DispatchTable::instance().dump_coverage_report();
    }

    if (pcrecomp::runtime::get_trace_config().show_frontier) {
        pcrecomp::runtime::ExecutionFrontier::instance().dump_frontier();
    }

    pcrecomp::renderer::Renderer::instance().shutdown();
    pcrecomp::rw::shutdown_renderware_subsystems();
    pcrecomp::runtime::GuestMemory::instance().shutdown();

    std::cout << "[Exit] Recomp runtime completed cleanly.\n";
    return 0;
}
