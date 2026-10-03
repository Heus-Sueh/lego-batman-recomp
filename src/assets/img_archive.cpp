#include "img_archive.h"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace pcrecomp::assets {

ImgArchive::ImgArchive() = default;

ImgArchive::~ImgArchive() {
    close();
}

std::string ImgArchive::normalize_name(const std::string& name) {
    std::string norm = name;
    std::transform(norm.begin(), norm.end(), norm.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return norm;
}

bool ImgArchive::open(const std::filesystem::path& img_path) {
    close();

    m_file.open(img_path, std::ios::binary);
    if (!m_file.is_open()) {
        std::cerr << "[ImgArchive] Could not open archive: " << img_path << "\n";
        return false;
    }

    m_path = img_path;

    char magic[4] = {0};
    m_file.read(magic, 4);
    if (std::memcmp(magic, "VER2", 4) != 0) {
        std::cerr << "[ImgArchive] Unsupported IMG header: " << std::string(magic, 4) << " in " << img_path << "\n";
        close();
        return false;
    }

    uint32_t num_entries = 0;
    m_file.read(reinterpret_cast<char*>(&num_entries), 4);
    if (num_entries == 0 || num_entries > 100000) {
        std::cerr << "[ImgArchive] Invalid entry count: " << num_entries << "\n";
        close();
        return false;
    }

    m_entries.reserve(num_entries);

    for (uint32_t i = 0; i < num_entries; ++i) {
        uint32_t offset = 0;
        uint32_t size = 0;
        char name_buf[24] = {0};

        m_file.read(reinterpret_cast<char*>(&offset), 4);
        m_file.read(reinterpret_cast<char*>(&size), 4);
        m_file.read(name_buf, 24);

        std::string name(name_buf);
        std::string key = normalize_name(name);

        m_entries[key] = ImgEntry{ offset, size, name };
    }

    std::cout << "[ImgArchive] Opened " << img_path.filename().string()
              << " (" << m_entries.size() << " entries loaded)\n";
    return true;
}

void ImgArchive::close() {
    if (m_file.is_open()) {
        m_file.close();
    }
    m_entries.clear();
    m_path.clear();
}

bool ImgArchive::has_file(const std::string& name) const noexcept {
    return m_entries.contains(normalize_name(name));
}

std::vector<std::string> ImgArchive::file_names() const {
    std::vector<std::string> names;
    names.reserve(m_entries.size());
    for (const auto& [_, entry] : m_entries) {
        names.push_back(entry.name);
    }
    return names;
}

std::optional<std::vector<uint8_t>> ImgArchive::read_file(const std::string& name) {
    if (!m_file.is_open()) return std::nullopt;

    auto it = m_entries.find(normalize_name(name));
    if (it == m_entries.end()) {
        return std::nullopt;
    }

    const auto& entry = it->second;
    size_t byte_offset = static_cast<size_t>(entry.offset_sectors) * 2048;
    size_t byte_size   = static_cast<size_t>(entry.size_sectors) * 2048;

    m_file.seekg(static_cast<std::streamoff>(byte_offset), std::ios::beg);
    if (!m_file) {
        std::cerr << "[ImgArchive] Failed seek to " << entry.name << " at offset " << byte_offset << "\n";
        return std::nullopt;
    }

    std::vector<uint8_t> buffer(byte_size);
    m_file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(byte_size));
    if (!m_file) {
        std::cerr << "[ImgArchive] Failed reading " << entry.name << "\n";
        return std::nullopt;
    }

    return buffer;
}

} // namespace pcrecomp::assets
