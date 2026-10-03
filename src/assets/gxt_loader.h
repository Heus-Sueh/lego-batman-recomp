#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace pcrecomp::assets {

class GxtLoader {
public:
    static GxtLoader& instance();

    bool load_file(const std::filesystem::path& gxt_path);

    [[nodiscard]] bool is_loaded() const noexcept { return !m_strings.empty(); }
    [[nodiscard]] size_t string_count() const noexcept { return m_strings.size(); }

    [[nodiscard]] bool has(const std::string& key) const;
    [[nodiscard]] std::string get(const std::string& key, const std::string& default_text = "") const;
    [[nodiscard]] static uint32_t calculate_key_crc(const std::string& key);

private:
    GxtLoader() = default;
    ~GxtLoader() = default;

    std::unordered_map<uint32_t, std::string> m_strings;
};

} // namespace pcrecomp::assets
