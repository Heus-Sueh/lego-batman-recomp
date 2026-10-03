#pragma once

#include "guest_memory.h"
#include <filesystem>
#include <string>
#include <vector>

namespace pcrecomp::runtime {

struct LoadedSectionInfo {
    std::string name;
    GuestAddress va{0};
    uint32_t virtual_size{0};
    uint32_t raw_size{0};
    bool loaded{false};
};

class SectionLoader {
public:
    // Loads .rdata, .data, and auxiliary data sections from the PE executable into GuestMemory
    static bool load_pe_data_sections(const std::filesystem::path& exe_path, GuestMemory& memory);

    // Returns the list of loaded sections from the last call
    static const std::vector<LoadedSectionInfo>& last_loaded_sections();

    // Verifies that data sections are populated with non-zero initialized data
    static bool verify_data_integrity(const GuestMemory& memory);
};

} // namespace pcrecomp::runtime
