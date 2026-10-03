#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "../rw/rw_core.h"

namespace pcrecomp::assets {

class TextureDictionary {
public:
    static TextureDictionary& instance();

    bool load_dictionary(const std::string& name, const std::filesystem::path& path);
    rw::RwTexture* find_texture(const std::string& name) const;
    void register_texture(const std::string& name, rw::RwTexture* tex);
    void clear();

    [[nodiscard]] size_t texture_count() const noexcept { return m_textures.size(); }
    [[nodiscard]] const std::unordered_map<std::string, rw::RwTexture*>& textures() const noexcept { return m_textures; }

private:
    TextureDictionary() = default;
    ~TextureDictionary() = default;

    static std::string to_lower(std::string_view str);

    std::unordered_map<std::string, rw::RwTexture*> m_textures;
};

class TxdLoader {
public:
    static std::unordered_map<std::string, rw::RwTexture*> load_from_file(const std::filesystem::path& path);
    static std::unordered_map<std::string, rw::RwTexture*> load_from_memory(const uint8_t* data, size_t size);

private:
    static void decode_dxt1(const uint8_t* src, int width, int height, uint32_t* dst_argb);
    static void decode_dxt3(const uint8_t* src, int width, int height, uint32_t* dst_argb);
    static void decode_dxt5(const uint8_t* src, int width, int height, uint32_t* dst_argb);
};

} // namespace pcrecomp::assets

