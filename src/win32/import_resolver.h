#pragma once

#include "win32_types.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace pcrecomp::win32 {

using Win32ApiFunction = void* (*)();

class ImportResolver {
public:
    static ImportResolver& instance();

    void initialize();

    // Resolves a function pointer by DLL module name and function name / ordinal
    [[nodiscard]] void* resolve_import(const std::string& module_name, const std::string& function_name) const;

    // Registers a shimmed implementation for an API
    void register_shim(const std::string& module_name, const std::string& function_name, void* fn_ptr);

    [[nodiscard]] size_t registered_count() const noexcept;

private:
    ImportResolver() = default;

    std::unordered_map<std::string, void*> m_shims;
};

// Global initialization helper
void initialize_win32_imports();

} // namespace pcrecomp::win32
