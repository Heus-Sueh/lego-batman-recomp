#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include <optional>

namespace pcmodern {

class VirtualFileSystem {
public:
    static VirtualFileSystem& instance();

    void set_root(const std::filesystem::path& root_path);
    [[nodiscard]] const std::filesystem::path& root() const noexcept { return m_root; }

    // Resolves a guest Windows path (e.g., "data\\archive.bin")
    // into an existing host filesystem path using case-insensitive matching.
    [[nodiscard]] std::optional<std::filesystem::path> resolve(const std::string& guest_path);

    [[nodiscard]] static std::string normalize_separators(const std::string& path);

private:
    VirtualFileSystem() = default;

    std::filesystem::path m_root{"."};
    std::unordered_map<std::string, std::filesystem::path> m_lookup_cache;
};

} // namespace pcmodern
