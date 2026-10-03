#pragma once

#include "guest_types.h"
#include <unordered_map>
#include <string>
#include <functional>

namespace pcrecomp::runtime {

using GuestFunctionFn = void (*)(GuestRegisters&);

struct RegisteredFunction {
    GuestFunctionFn fn{nullptr};
    std::string     name;
    uint64_t        call_count{0};
};

class DispatchTable {
public:
    static DispatchTable& instance();

    void register_function(GuestAddress addr, GuestFunctionFn fn, const std::string& name = "");
    bool has_function(GuestAddress addr) const;
    const RegisteredFunction* get_function(GuestAddress addr) const;

    bool dispatch(GuestAddress addr, GuestRegisters& regs, GuestAddress caller = 0);
    void record_call(GuestAddress addr);
    void log_unresolved(GuestAddress caller, GuestAddress target, const GuestRegisters& regs, const char* context = "");

    void set_abort_on_unresolved(bool abort_on_unresolved) noexcept { m_abort_on_unresolved = abort_on_unresolved; }
    [[nodiscard]] bool abort_on_unresolved() const noexcept { return m_abort_on_unresolved; }

    [[nodiscard]] uint64_t call_count(GuestAddress addr) const;
    [[nodiscard]] size_t executed_function_count() const;
    [[nodiscard]] size_t registered_function_count() const;
    [[nodiscard]] uint64_t total_dispatches() const noexcept { return m_total_dispatches; }
    [[nodiscard]] uint64_t unresolved_count() const noexcept { return m_unresolved_count; }
    void dump_coverage_report() const;

private:
    DispatchTable();
    ~DispatchTable();

    std::unordered_map<GuestAddress, RegisteredFunction> m_table;
    std::string m_log_path{"logs/unresolved_calls.log"};
    bool m_abort_on_unresolved{false};
    uint64_t m_total_dispatches{0};
    uint64_t m_unresolved_count{0};
};

} // namespace pcrecomp::runtime
