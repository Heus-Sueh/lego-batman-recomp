#pragma once

#include "guest_types.h"
#include <cstdint>
#include <string>
#include <stdexcept>

namespace pcrecomp::runtime {

struct FrameLimitReached final : std::exception {
    const char* what() const noexcept override { return "bounded execution frame limit reached"; }
};

struct TraceConfig {
    bool trace_boot{false};
    bool trace_imports{false};
    bool trace_icalls{false};
    bool trace_functions{false};
    bool trace_files{false};
    bool trace_threads{false};
    bool show_frontier{false};
};

class ExecutionFrontier {
public:
    static ExecutionFrontier& instance();

    void record_successful_entry(GuestAddress va);
    void record_failure(GuestAddress current_va, GuestAddress target_va, const std::string& reason, const std::string& classification);

    [[nodiscard]] GuestAddress last_successful_va() const noexcept { return m_last_successful; }
    [[nodiscard]] GuestAddress current_va() const noexcept { return m_current; }
    [[nodiscard]] GuestAddress target_va() const noexcept { return m_target; }
    [[nodiscard]] const std::string& failure_reason() const noexcept { return m_reason; }
    [[nodiscard]] const std::string& classification() const noexcept { return m_classification; }

    void dump_frontier() const;

private:
    ExecutionFrontier() = default;

    GuestAddress m_last_successful{0};
    GuestAddress m_current{0};
    GuestAddress m_target{0};
    std::string  m_reason{"None"};
    std::string  m_classification{"None"};
};

inline TraceConfig& get_trace_config() {
    static TraceConfig s_config;
    return s_config;
}

} // namespace pcrecomp::runtime
