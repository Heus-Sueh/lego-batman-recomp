#include "dispatch.h"
#include "diagnostics.h"
#include "bridge_adapter.h"
#include <pcrecomp/dispatch.h>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>

namespace pcrecomp::runtime {
namespace { thread_local std::exception_ptr guest_exception; }

DispatchTable& DispatchTable::instance() {
    static DispatchTable s_instance;
    return s_instance;
}

DispatchTable::DispatchTable() { (void)bridge_runtime(); }
DispatchTable::~DispatchTable() = default;

void DispatchTable::register_function(GuestAddress addr, GuestFunctionFn fn, const std::string& name) {
    m_table[addr] = RegisteredFunction{fn, name, 0};
    auto status = pc_register_guest_function(bridge_runtime(),addr,
        [](PcRuntime*,PcContext* ctx,void* data) -> PcStatus {
            try { static_cast<RegisteredFunction*>(data)->fn(*ctx); return PC_OK; }
            catch (const FrameLimitReached&) { guest_exception=std::current_exception(); return PC_YIELDED; }
            catch (...) { guest_exception=std::current_exception(); return PC_INTERNAL_ERROR; }
        }, &m_table.at(addr));
    if (status != PC_OK && status != PC_DUPLICATE)
        throw std::runtime_error("Guest function registration failed");
}

bool DispatchTable::has_function(GuestAddress addr) const {
    return m_table.find(addr) != m_table.end();
}

const RegisteredFunction* DispatchTable::get_function(GuestAddress addr) const {
    auto it = m_table.find(addr);
    if (it != m_table.end()) {
        return &it->second;
    }
    return nullptr;
}

bool DispatchTable::dispatch(GuestAddress addr, GuestRegisters& regs, GuestAddress caller) {
    ++m_total_dispatches;
    auto it = m_table.find(addr);
    if (it != m_table.end() && it->second.fn) {
        if (get_trace_config().trace_icalls) {
            std::cout << "[TRACE:ICALL] 0x" << std::hex << std::setw(8) << std::setfill('0') << caller
                      << " -> 0x" << std::setw(8) << std::setfill('0') << addr
                      << " (" << it->second.name << ")\n" << std::dec;
        }
        ExecutionFrontier::instance().record_successful_entry(addr);
        ++it->second.call_count;
        const auto status = pc_call_indirect(bridge_runtime(),addr,&regs,caller);
        if (guest_exception) { auto e=guest_exception; guest_exception=nullptr; std::rethrow_exception(e); }
        return status == PC_OK;
    }

    pc_call_indirect(bridge_runtime(),addr,&regs,caller);
    ExecutionFrontier::instance().record_failure(caller, addr, "Unmapped indirect call", "UNKNOWN");
    log_unresolved(caller, addr, regs, "unmapped_dispatch");
    return false;
}

void DispatchTable::record_call(GuestAddress addr) {
    ++m_total_dispatches;
    auto it = m_table.find(addr);
    if (it != m_table.end()) {
        ++it->second.call_count;
    }
}

uint64_t DispatchTable::call_count(GuestAddress addr) const {
    auto it = m_table.find(addr);
    if (it != m_table.end()) {
        return it->second.call_count;
    }
    return 0;
}

size_t DispatchTable::executed_function_count() const {
    size_t count = 0;
    for (const auto& [addr, rf] : m_table) {
        if (rf.call_count > 0) {
            ++count;
        }
    }
    return count;
}

size_t DispatchTable::registered_function_count() const {
    return m_table.size();
}

void DispatchTable::dump_coverage_report() const {
    size_t registered = registered_function_count();
    size_t executed = executed_function_count();
    double pct = registered > 0 ? (100.0 * executed / static_cast<double>(registered)) : 0.0;

    std::cout << "\n============================================================\n";
    std::cout << "  pcrecomp Runtime Coverage Report (Step 20 / Section 10)\n";
    std::cout << "============================================================\n";
    std::cout << "  Functions registered:          " << registered << "\n";
    std::cout << "  Functions runtime reached:     " << executed << " (" << std::fixed << std::setprecision(1) << pct << "%)\n";
    std::cout << "  Total function dispatches:     " << m_total_dispatches << "\n";
    std::cout << "  Unresolved indirect calls:     " << m_unresolved_count << "\n";
    std::cout << "  Unreached registered functions:\n";
    size_t unreached_cnt = 0;
    for (const auto& [addr, rf] : m_table) {
        if (rf.call_count == 0) {
            std::cout << "    [ ] 0x" << std::hex << std::setw(8) << std::setfill('0') << addr << " " << rf.name << std::dec << "\n";
            if (++unreached_cnt >= 25) break;
        }
    }
    std::cout << "============================================================\n\n" << std::dec;
}

void DispatchTable::log_unresolved(GuestAddress caller, GuestAddress target, const GuestRegisters& regs, const char* context) {
    ++m_unresolved_count;
    try {
        std::filesystem::create_directories("logs");
        std::ofstream log_file(m_log_path, std::ios::app);
        if (log_file.is_open()) {
            log_file << "UNRESOLVED_CALL: caller=0x" << std::hex << std::setw(8) << std::setfill('0') << caller
                     << " target=0x" << std::setw(8) << std::setfill('0') << target
                     << " context=" << context
                     << " EAX=0x" << regs.eax << " ECX=0x" << regs.ecx << " EDX=0x" << regs.edx
                     << " EBX=0x" << regs.ebx << " ESP=0x" << regs.esp << " EBP=0x" << regs.ebp
                     << " ESI=0x" << regs.esi << " EDI=0x" << regs.edi
                     << std::dec << "\n";
        }
    } catch (...) {
        // Fallback to stderr if filesystem write fails
    }

    std::cerr << "[DISPATCH] CRITICAL: Unresolved indirect call from 0x" 
              << std::hex << caller << " to target 0x" << target 
              << " (" << context << ") - logged to " << m_log_path << std::dec << "\n";

    if (m_abort_on_unresolved) {
        std::cerr << "[DISPATCH] Aborting due to unresolved indirect call (Section 12.2 policy).\n";
        std::abort();
    }
}

} // namespace pcrecomp::runtime

extern "C" {

extern uint32_t g_enter_trace[1024];
extern uint32_t g_enter_idx;

void recomp_trace_enter(uint32_t va) {
    g_enter_trace[g_enter_idx & (1024 - 1)] = va;
    ++g_enter_idx;
    pcrecomp::runtime::DispatchTable::instance().record_call(va);
    pcrecomp::runtime::ExecutionFrontier::instance().record_successful_entry(va);
    if (pcrecomp::runtime::get_trace_config().trace_functions) {
        std::cout << "[TRACE:FUNC] 0x" << std::hex << std::setw(8) << std::setfill('0') << va << std::dec << "\n";
    }
}

void recomp_dump_trace(const char* why) {
    std::cerr << "[TRACE:DUMP] Execution fault / trace dump requested: " << (why ? why : "unknown") << "\n";
    pcrecomp::runtime::ExecutionFrontier::instance().dump_frontier();
}

} // extern "C"
