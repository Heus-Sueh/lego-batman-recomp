#include <pcmodern/filesystem.h>
#include <pcmodern/time.h>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace pcmodern {

VirtualFileSystem& VirtualFileSystem::instance() {
    static VirtualFileSystem s_instance;
    return s_instance;
}

void VirtualFileSystem::set_root(const std::filesystem::path& root_path) {
    m_root = root_path;
    m_lookup_cache.clear();
    log("filesystem", "Root set to: " + m_root.string());
}

std::string VirtualFileSystem::normalize_separators(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

static bool string_equals_ci(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

std::optional<std::filesystem::path> VirtualFileSystem::resolve(const std::string& guest_path) {
    std::string normalized = normalize_separators(guest_path);

    // Check lookup cache first
    auto it = m_lookup_cache.find(normalized);
    if (it != m_lookup_cache.end()) {
        return it->second;
    }

    std::filesystem::path current = m_root;
    std::filesystem::path rel_path(normalized);

    // If path is absolute on Windows (e.g., C:/application/data), strip drive letter
    auto it_elem = rel_path.begin();
    if (it_elem != rel_path.end() && it_elem->string().find(':') != std::string::npos) {
        ++it_elem;
    }

    bool found = true;
    for (; it_elem != rel_path.end(); ++it_elem) {
        const std::string part = it_elem->string();
        if (part == "." || part.empty()) continue;

        // Try exact match first
        std::filesystem::path candidate = current / part;
        if (std::filesystem::exists(candidate)) {
            current = candidate;
            continue;
        }

        // Search directory for case-insensitive match
        bool part_matched = false;
        if (std::filesystem::is_directory(current)) {
            for (const auto& entry : std::filesystem::directory_iterator(current)) {
                if (string_equals_ci(entry.path().filename().string(), part)) {
                    current = entry.path();
                    part_matched = true;
                    break;
                }
            }
        }

        if (!part_matched) {
            found = false;
            break;
        }
    }

    if (found && std::filesystem::exists(current)) {
        m_lookup_cache[normalized] = current;
        return current;
    }

    return std::nullopt;
}

} // namespace pcmodern
