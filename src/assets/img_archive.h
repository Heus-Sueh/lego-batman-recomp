#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <optional>

namespace pcrecomp::assets {

struct ImgEntry {
    uint32_t offset_sectors{0};
    uint32_t size_sectors{0};
    std::string name;
};

class ImgArchive {
public:
    ImgArchive();
    ~ImgArchive();

    bool open(const std::filesystem::path& img_path);
    void close();

    [[nodiscard]] bool is_open() const noexcept { return m_file.is_open(); }
    [[nodiscard]] bool has_file(const std::string& name) const noexcept;
    [[nodiscard]] size_t entry_count() const noexcept { return m_entries.size(); }
    [[nodiscard]] std::vector<std::string> file_names() const;

    std::optional<std::vector<uint8_t>> read_file(const std::string& name);

private:
    static std::string normalize_name(const std::string& name);

    mutable std::ifstream m_file;
    std::filesystem::path m_path;
    std::unordered_map<std::string, ImgEntry> m_entries;
};

} // namespace pcrecomp::assets
