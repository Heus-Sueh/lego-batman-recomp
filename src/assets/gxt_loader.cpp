#include "gxt_loader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cctype>

namespace pcrecomp::assets {

GxtLoader& GxtLoader::instance() {
    static GxtLoader s_instance;
    return s_instance;
}

uint32_t GxtLoader::calculate_key_crc(const std::string& key) {
    uint32_t crc = 0xFFFFFFFFu;
    for (char ch : key) {
        auto c = static_cast<uint8_t>(std::toupper(static_cast<unsigned char>(ch)));
        crc ^= c;
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool GxtLoader::load_file(const std::filesystem::path& gxt_path) {
    m_strings.clear();

    std::ifstream file(gxt_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[GxtLoader] Could not open GXT file: " << gxt_path << "\n";
        return false;
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));

    const uint8_t* data = buffer.data();
    size_t size = buffer.size();

    if (size < 12) return false;

    char magic[5] = {0};
    std::memcpy(magic, data + 4, 4);
    if (std::strcmp(magic, "TABL") != 0) {
        std::cerr << "[GxtLoader] Not a valid TABL GXT file: " << magic << "\n";
        return false;
    }

    uint32_t tabl_size = *reinterpret_cast<const uint32_t*>(data + 8);
    size_t num_tables = tabl_size / 12;

    struct TableInfo {
        std::string name;
        uint32_t offset;
    };
    std::vector<TableInfo> tables;
    tables.reserve(num_tables);

    size_t tpos = 12;
    for (size_t i = 0; i < num_tables && tpos + 12 <= size; ++i) {
        char tname[9] = {0};
        std::memcpy(tname, data + tpos, 8);
        uint32_t offset = *reinterpret_cast<const uint32_t*>(data + tpos + 8);
        tables.push_back({ std::string(tname), offset });
        tpos += 12;
    }

    for (const auto& tbl : tables) {
        size_t pos = tbl.offset;
        if (pos >= size) continue;

        // Subsequent tables have an 8-byte table name prefix before TKEY
        if (tbl.name != "MAIN") {
            pos += 8;
        }

        if (pos + 8 > size) continue;

        char tkey_magic[5] = {0};
        std::memcpy(tkey_magic, data + pos, 4);
        if (std::strcmp(tkey_magic, "TKEY") != 0) continue;

        uint32_t tkey_size = *reinterpret_cast<const uint32_t*>(data + pos + 4);
        pos += 8;

        size_t num_keys = tkey_size / 8;
        struct KeyEntry {
            uint32_t offset;
            uint32_t crc;
        };
        std::vector<KeyEntry> key_entries;
        key_entries.reserve(num_keys);

        for (size_t k = 0; k < num_keys && pos + 8 <= size; ++k) {
            uint32_t off = *reinterpret_cast<const uint32_t*>(data + pos);
            uint32_t crc = *reinterpret_cast<const uint32_t*>(data + pos + 4);
            key_entries.push_back({ off, crc });
            pos += 8;
        }

        if (pos + 8 > size) continue;
        char tdat_magic[5] = {0};
        std::memcpy(tdat_magic, data + pos, 4);
        if (std::strcmp(tdat_magic, "TDAT") != 0) continue;

        uint32_t tdat_size = *reinterpret_cast<const uint32_t*>(data + pos + 4);
        (void)tdat_size;
        size_t tdat_pos = pos + 8;

        for (const auto& entry : key_entries) {
            size_t str_pos = tdat_pos + entry.offset;
            if (str_pos >= size) continue;

            std::string str;
            while (str_pos < size && data[str_pos] != 0) {
                // Strip GTA color codes: ~w~, ~r~, ~y~, ~g~, ~b~, ~s~, ~h~
                if (data[str_pos] == '~' && str_pos + 2 < size && data[str_pos + 2] == '~') {
                    char code = static_cast<char>(data[str_pos + 1]);
                    if (code == 'n' || code == 'N') {
                        str.push_back('\n');
                    }
                    str_pos += 3;
                } else {
                    str.push_back(static_cast<char>(data[str_pos]));
                    ++str_pos;
                }
            }

            m_strings[entry.crc] = str;
        }
    }

    std::cout << "[GxtLoader] Loaded " << m_strings.size()
              << " localized strings from " << gxt_path.filename().string() << "\n";
    return true;
}

bool GxtLoader::has(const std::string& key) const {
    uint32_t crc = calculate_key_crc(key);
    return m_strings.contains(crc);
}

std::string GxtLoader::get(const std::string& key, const std::string& default_text) const {
    uint32_t crc = calculate_key_crc(key);
    auto it = m_strings.find(crc);
    if (it != m_strings.end()) {
        return it->second;
    }
    return default_text.empty() ? key : default_text;
}

} // namespace pcrecomp::assets
