#include "adapters/adapter.h"
#include "target_config.h"
#include "lego_revision.h"
#include "runtime/guest_memory.h"
#include "runtime/section_loader.h"
#include "runtime/dispatch.h"
#include "runtime/diagnostics.h"
#include "platform/platform.h"
#include "platform/vfs.h"
#include "win32/win32_shims.h"
#include "win32/import_resolver.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

extern void register_generated_recomp_functions();
extern "C" { extern uint32_t g_esp; extern int32_t g_max_frames; }

namespace {
uint32_t read_le(const std::vector<unsigned char>& data, size_t offset, size_t count) {
    if (offset > data.size() || count > data.size() - offset)
        throw std::runtime_error("Truncated PE header");
    uint32_t value = 0;
    for (size_t i = 0; i < count; ++i) value |= uint32_t(data[offset + i]) << (8 * i);
    return value;
}
void validate_image(const std::filesystem::path& path) {
    if (!std::filesystem::is_regular_file(path) ||
        std::filesystem::file_size(path) != lego_revision::file_size)
        throw std::runtime_error("Missing executable or revision file-size mismatch: " + path.string());
    std::ifstream file(path, std::ios::binary);
    std::vector<unsigned char> header(4096);
    file.read(reinterpret_cast<char*>(header.data()), header.size());
    header.resize(static_cast<size_t>(file.gcount()));
    const size_t pe = read_le(header, 0x3c, 4);
    if (read_le(header, 0, 2) != 0x5a4d || read_le(header, pe, 4) != 0x4550 ||
        read_le(header, pe + 4, 2) != 0x14c || read_le(header, pe + 24, 2) != 0x10b ||
        read_le(header, pe + 52, 4) != lego_revision::image_base ||
        uint64_t(read_le(header, pe + 40, 4)) + lego_revision::image_base != lego_revision::entry)
        throw std::runtime_error("Executable PE identity does not match the selected revision");
}
struct Cleanup {
    ~Cleanup() {
        pcrecomp::platform::Platform::instance().shutdown();
        pcrecomp::runtime::GuestMemory::instance().shutdown();
    }
};
}

int pcrecomp::adapter::run(int argc, char* argv[]) {
    using namespace pcrecomp;
    bool headless = false, coverage = false, trace_boot = false, test_dispatch = false;
    bool test_imports = false, test_unresolved = false;
    int frames = -1;
    std::filesystem::path root = target::default_game_dir;
    platform::WindowConfig window;
    window.title = target::title;
    window.width = target::default_window_width;
    window.height = target::default_window_height;
    std::cout << target::title << " — adapter: lego_batman\n";
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            auto value = [&]() -> std::string {
                if (i + 1 >= argc) throw std::runtime_error("Missing value for " + arg);
                return argv[++i];
            };
            auto positive = [&]() {
                const auto text = value();
                size_t end = 0;
                int n = std::stoi(text, &end);
                if (n <= 0 || end != text.size()) throw std::runtime_error("Expected positive integer for " + arg);
                return n;
            };
            auto& trace = runtime::get_trace_config();
            if (arg == "--help" || arg == "-h") {
                std::cout << "Options: --game-dir PATH --headless --frames N --width N --height N\n"
                    "  --windowed --fullscreen --coverage --frontier --version\n"
                    "  --trace-boot --trace-imports --trace-icalls --trace-functions --trace-files --trace-threads\n"
                    "  --test-dispatch --test-imports --test-unresolved\n"
                    "Frame bounds count message-pump polls, not rendered game frames.\n";
                return 0;
            } else if (arg == "--version" || arg == "-v") return 0;
            else if (arg == "--game-dir") root = value();
            else if (arg == "--headless") headless = true;
            else if (arg == "--frames") frames = positive();
            else if (arg == "--width") window.width = positive();
            else if (arg == "--height") window.height = positive();
            else if (arg == "--fullscreen") window.fullscreen = true;
            else if (arg == "--windowed") window.fullscreen = false;
            else if (arg == "--coverage") coverage = true;
            else if (arg == "--frontier") trace.show_frontier = true;
            else if (arg == "--trace-boot") { trace_boot = true; trace.trace_boot = true; }
            else if (arg == "--trace-imports") trace.trace_imports = true;
            else if (arg == "--trace-icalls") trace.trace_icalls = true;
            else if (arg == "--trace-functions") trace.trace_functions = true;
            else if (arg == "--trace-files") trace.trace_files = true;
            else if (arg == "--trace-threads") trace.trace_threads = true;
            else if (arg == "--test-dispatch") test_dispatch = true;
            else if (arg == "--test-imports") test_imports = true;
            else if (arg == "--test-unresolved") test_unresolved = true;
            else throw std::runtime_error("Unsupported option for lego_batman adapter: " + arg);
        }
        const auto exe = root / target::guest_executable;
        validate_image(exe);
        std::cout << "[Init] Discovered game installation at: " << root << "\n";
        platform::VirtualFileSystem::instance().set_root(root);
        Cleanup cleanup;
        auto& memory = runtime::GuestMemory::instance();
        if (!memory.initialize() || !runtime::SectionLoader::load_pe_data_sections(exe, memory))
            throw std::runtime_error("Guest image initialization failed");
        register_generated_recomp_functions();
        auto& dispatch = runtime::DispatchTable::instance();
        if (!dispatch.has_function(lego_revision::entry))
            throw std::runtime_error("Selected PE entry is absent from generated dispatch");
        win32::initialize_win32_subsystems();
        win32::initialize_win32_imports();
        if (headless) platform::Platform::instance().set_running(true);
        else if (!platform::Platform::instance().initialize(window))
            throw std::runtime_error("Platform initialization failed");
        g_esp = lego_revision::stack_top;
        g_max_frames = frames > 0 ? frames : ((headless || trace_boot || test_dispatch) ? 1 : -1);
        if (test_imports) {
            auto& resolver = win32::ImportResolver::instance();
            if (!resolver.resolve_import("KERNEL32.dll", "CreateFileA"))
                throw std::runtime_error("CreateFileA import missing");
            std::cout << "[Test] Import shims resolved successfully\n";
        } else if (test_unresolved) {
            runtime::GuestRegisters regs{};
            dispatch.dispatch(0xdeadbeefu, regs, lego_revision::entry);
            std::cout << "[Test] Unresolved call diagnostic completed\n";
        } else {
            std::cout << (test_dispatch ? "[Test] Executing recompiled entry point dispatch (" :
                "[Init] Entering original PE entry point: mainCRTStartup (");
            std::cout << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
                      << lego_revision::entry << std::dec << std::nouppercase << ")...\n";
            runtime::GuestRegisters regs{};
            try {
                if (!dispatch.dispatch(lego_revision::entry, regs, 0))
                    throw std::runtime_error("Guest entry dispatch failed");
            } catch (const runtime::FrameLimitReached&) {
                std::cout << "[Runtime] Bounded message-pump limit reached; leaving guest execution.\n";
            }
            if (test_dispatch) std::cout << "[Test] Dispatch execution completed successfully.\n";
            if (trace_boot) std::cout << "[TraceBoot] Original PE entry point execution completed successfully.\n";
        }
        if (coverage) dispatch.dump_coverage_report();
        if (runtime::get_trace_config().show_frontier) runtime::ExecutionFrontier::instance().dump_frontier();
        if (!test_unresolved && dispatch.unresolved_count())
            throw std::runtime_error("Guest execution encountered unresolved calls");
        std::cout << "[Exit] Recomp runtime completed cleanly.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[Fatal] " << error.what() << "\n";
        if (coverage) runtime::DispatchTable::instance().dump_coverage_report();
        if (runtime::get_trace_config().show_frontier)
            runtime::ExecutionFrontier::instance().dump_frontier();
        return 1;
    }
}
