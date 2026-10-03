#include <cstdint>
extern "C" {
int32_t g_max_frames = -1;
uint32_t g_frame_counter = 0;
}

#include "diagnostics.h"
#include <iostream>
#include <iomanip>

namespace pcrecomp::runtime {

ExecutionFrontier& ExecutionFrontier::instance() {
    static ExecutionFrontier s_instance;
    return s_instance;
}

void ExecutionFrontier::record_successful_entry(GuestAddress va) {
    m_last_successful = m_current;
    m_current = va;
}

void ExecutionFrontier::record_failure(GuestAddress current_va, GuestAddress target_va, const std::string& reason, const std::string& classification) {
    m_current = current_va;
    m_target = target_va;
    m_reason = reason;
    m_classification = classification;
}

void ExecutionFrontier::dump_frontier() const {
    std::cout << "\n============================================================\n";
    std::cout << "  pcrecomp Execution Frontier Status\n";
    std::cout << "============================================================\n";
    std::cout << "  Last Successful VA: 0x" << std::hex << std::setw(8) << std::setfill('0') << m_last_successful << "\n";
    std::cout << "  Current VA:         0x" << std::setw(8) << std::setfill('0') << m_current << "\n";
    std::cout << "  Target VA:          0x" << std::setw(8) << std::setfill('0') << m_target << "\n" << std::dec;
    std::cout << "  Classification:     " << m_classification << "\n";
    std::cout << "  Failure Reason:     " << m_reason << "\n";
    std::cout << "============================================================\n\n";
}

} // namespace pcrecomp::runtime
