#include "txd_loader.h"
#include "../platform/vfs.h"
#include "../runtime/guest_memory.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cctype>

namespace pcrecomp::assets {

namespace {

constexpr uint32_t rwID_STRUCT         = 0x01;
constexpr uint32_t rwID_TEXTURENATIVE  = 0x15;
constexpr uint32_t rwID_TEXDICTIONARY  = 0x16;

inline uint16_t read_u16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

inline uint32_t read_u32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

inline uint64_t read_u48(const uint8_t* p) {
    return static_cast<uint64_t>(p[0]) |
           (static_cast<uint64_t>(p[1]) << 8) |
           (static_cast<uint64_t>(p[2]) << 16) |
           (static_cast<uint64_t>(p[3]) << 24) |
           (static_cast<uint64_t>(p[4]) << 32) |
           (static_cast<uint64_t>(p[5]) << 40);
}

} // namespace

void TxdLoader::decode_dxt1(const uint8_t* src, int width, int height, uint32_t* dst_argb) {
    int blocks_x = (width + 3) / 4;
    int blocks_y = (height + 3) / 4;
    size_t offset = 0;

    for (int by = 0; by < blocks_y; ++by) {
        for (int bx = 0; bx < blocks_x; ++bx) {
            uint16_t c0 = read_u16(src + offset);
            uint16_t c1 = read_u16(src + offset + 2);
            uint32_t bits = read_u32(src + offset + 4);
            offset += 8;

            uint32_t r0 = ((c0 >> 11) & 0x1F) * 255 / 31;
            uint32_t g0 = ((c0 >> 5)  & 0x3F) * 255 / 63;
            uint32_t b0 = (c0         & 0x1F) * 255 / 31;

            uint32_t r1 = ((c1 >> 11) & 0x1F) * 255 / 31;
            uint32_t g1 = ((c1 >> 5)  & 0x3F) * 255 / 63;
            uint32_t b1 = (c1         & 0x1F) * 255 / 31;

            uint32_t col[4];
            col[0] = (0xFFu << 24) | (r0 << 16) | (g0 << 8) | b0;
            col[1] = (0xFFu << 24) | (r1 << 16) | (g1 << 8) | b1;

            if (c0 > c1) {
                uint32_t r2 = (2 * r0 + r1) / 3;
                uint32_t g2 = (2 * g0 + g1) / 3;
                uint32_t b2 = (2 * b0 + b1) / 3;
                col[2] = (0xFFu << 24) | (r2 << 16) | (g2 << 8) | b2;

                uint32_t r3 = (r0 + 2 * r1) / 3;
                uint32_t g3 = (g0 + 2 * g1) / 3;
                uint32_t b3 = (b0 + 2 * b1) / 3;
                col[3] = (0xFFu << 24) | (r3 << 16) | (g3 << 8) | b3;
            } else {
                uint32_t r2 = (r0 + r1) / 2;
                uint32_t g2 = (g0 + g1) / 2;
                uint32_t b2 = (b0 + b1) / 2;
                col[2] = (0xFFu << 24) | (r2 << 16) | (g2 << 8) | b2;
                col[3] = 0x00000000u; // Transparent
            }

            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    int x = bx * 4 + px;
                    int y = by * 4 + py;
                    if (x < width && y < height) {
                        int p_idx = py * 4 + px;
                        uint32_t code = (bits >> (2 * p_idx)) & 3;
                        dst_argb[y * width + x] = col[code];
                    }
                }
            }
        }
    }
}

void TxdLoader::decode_dxt3(const uint8_t* src, int width, int height, uint32_t* dst_argb) {
    int blocks_x = (width + 3) / 4;
    int blocks_y = (height + 3) / 4;
    size_t offset = 0;

    for (int by = 0; by < blocks_y; ++by) {
        for (int bx = 0; bx < blocks_x; ++bx) {
            const uint8_t* alpha_bytes = src + offset;
            offset += 8;

            uint16_t c0 = read_u16(src + offset);
            uint16_t c1 = read_u16(src + offset + 2);
            uint32_t bits = read_u32(src + offset + 4);
            offset += 8;

            uint32_t r0 = ((c0 >> 11) & 0x1F) * 255 / 31;
            uint32_t g0 = ((c0 >> 5)  & 0x3F) * 255 / 63;
            uint32_t b0 = (c0         & 0x1F) * 255 / 31;

            uint32_t r1 = ((c1 >> 11) & 0x1F) * 255 / 31;
            uint32_t g1 = ((c1 >> 5)  & 0x3F) * 255 / 63;
            uint32_t b1 = (c1         & 0x1F) * 255 / 31;

            uint32_t rgb[4];
            rgb[0] = (r0 << 16) | (g0 << 8) | b0;
            rgb[1] = (r1 << 16) | (g1 << 8) | b1;
            rgb[2] = (((2 * r0 + r1) / 3) << 16) | (((2 * g0 + g1) / 3) << 8) | ((2 * b0 + b1) / 3);
            rgb[3] = (((r0 + 2 * r1) / 3) << 16) | (((g0 + 2 * g1) / 3) << 8) | ((b0 + 2 * b1) / 3);

            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    int x = bx * 4 + px;
                    int y = by * 4 + py;
                    if (x < width && y < height) {
                        int p_idx = py * 4 + px;
                        uint8_t a_byte = alpha_bytes[p_idx / 2];
                        uint32_t a = (p_idx % 2 == 0) ? ((a_byte & 0x0F) * 17) : (((a_byte >> 4) & 0x0F) * 17);

                        uint32_t code = (bits >> (2 * p_idx)) & 3;
                        dst_argb[y * width + x] = (a << 24) | rgb[code];
                    }
                }
            }
        }
    }
}

void TxdLoader::decode_dxt5(const uint8_t* src, int width, int height, uint32_t* dst_argb) {
    int blocks_x = (width + 3) / 4;
    int blocks_y = (height + 3) / 4;
    size_t offset = 0;

    for (int by = 0; by < blocks_y; ++by) {
        for (int bx = 0; bx < blocks_x; ++bx) {
            uint8_t a0 = src[offset];
            uint8_t a1 = src[offset + 1];
            uint64_t a_bits = read_u48(src + offset + 2);
            offset += 8;

            uint8_t alphas[8];
            alphas[0] = a0;
            alphas[1] = a1;
            if (a0 > a1) {
                alphas[2] = static_cast<uint8_t>((6 * a0 + 1 * a1) / 7);
                alphas[3] = static_cast<uint8_t>((5 * a0 + 2 * a1) / 7);
                alphas[4] = static_cast<uint8_t>((4 * a0 + 3 * a1) / 7);
                alphas[5] = static_cast<uint8_t>((3 * a0 + 4 * a1) / 7);
                alphas[6] = static_cast<uint8_t>((2 * a0 + 5 * a1) / 7);
                alphas[7] = static_cast<uint8_t>((1 * a0 + 6 * a1) / 7);
            } else {
                alphas[2] = static_cast<uint8_t>((4 * a0 + 1 * a1) / 5);
                alphas[3] = static_cast<uint8_t>((3 * a0 + 2 * a1) / 5);
                alphas[4] = static_cast<uint8_t>((2 * a0 + 3 * a1) / 5);
                alphas[5] = static_cast<uint8_t>((1 * a0 + 4 * a1) / 5);
                alphas[6] = 0;
                alphas[7] = 255;
            }

            uint16_t c0 = read_u16(src + offset);
            uint16_t c1 = read_u16(src + offset + 2);
            uint32_t bits = read_u32(src + offset + 4);
            offset += 8;

            uint32_t r0 = ((c0 >> 11) & 0x1F) * 255 / 31;
            uint32_t g0 = ((c0 >> 5)  & 0x3F) * 255 / 63;
            uint32_t b0 = (c0         & 0x1F) * 255 / 31;

            uint32_t r1 = ((c1 >> 11) & 0x1F) * 255 / 31;
            uint32_t g1 = ((c1 >> 5)  & 0x3F) * 255 / 63;
            uint32_t b1 = (c1         & 0x1F) * 255 / 31;

            uint32_t rgb[4];
            rgb[0] = (r0 << 16) | (g0 << 8) | b0;
            rgb[1] = (r1 << 16) | (g1 << 8) | b1;
            rgb[2] = (((2 * r0 + r1) / 3) << 16) | (((2 * g0 + g1) / 3) << 8) | ((2 * b0 + b1) / 3);
            rgb[3] = (((r0 + 2 * r1) / 3) << 16) | (((g0 + 2 * g1) / 3) << 8) | ((b0 + 2 * b1) / 3);

            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    int x = bx * 4 + px;
                    int y = by * 4 + py;
                    if (x < width && y < height) {
                        int p_idx = py * 4 + px;
                        uint32_t a_code = (a_bits >> (3 * p_idx)) & 7;
                        uint32_t a = alphas[a_code];

                        uint32_t code = (bits >> (2 * p_idx)) & 3;
                        dst_argb[y * width + x] = (a << 24) | rgb[code];
                    }
                }
            }
        }
    }
}

std::unordered_map<std::string, rw::RwTexture*> TxdLoader::load_from_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[TxdLoader] Could not open TXD file: " << path << "\n";
        return {};
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));

    return load_from_memory(buffer.data(), buffer.size());
}

std::unordered_map<std::string, rw::RwTexture*> TxdLoader::load_from_memory(const uint8_t* data, size_t size) {
    std::unordered_map<std::string, rw::RwTexture*> result;
    if (!data || size < 24) return result;

    uint32_t root_id = read_u32(data);
    (void)read_u32(data + 4);
    if (root_id != rwID_TEXDICTIONARY) {
        std::cerr << "[TxdLoader] Not a valid RenderWare TXD chunk: 0x" << std::hex << root_id << "\n";
        return result;
    }

    size_t pos = 12; // Skip root chunk header
    if (pos + 12 > size) return result;

    // Struct in TexDictionary
    uint32_t struct_id = read_u32(data + pos);
    uint32_t struct_size = read_u32(data + pos + 4);
    if (struct_id != rwID_STRUCT) return result;

    uint16_t tex_count = read_u16(data + pos + 12);
    pos += 12 + struct_size;

    for (uint16_t t = 0; t < tex_count && pos + 12 <= size; ++t) {
        uint32_t native_id = read_u32(data + pos);
        uint32_t native_size = read_u32(data + pos + 4);
        size_t next_pos = pos + 12 + native_size;

        if (native_id == rwID_TEXTURENATIVE) {
            size_t npos = pos + 12;
            if (npos + 12 <= size) {
                uint32_t s_id = read_u32(data + npos);
                uint32_t s_sz = read_u32(data + npos + 4);
                if (s_id == rwID_STRUCT && npos + 12 + s_sz <= size) {
                    const uint8_t* sdata = data + npos + 12;

                    char name_buf[33] = {0};
                    std::memcpy(name_buf, sdata + 8, 32);
                    std::string tex_name(name_buf);

                    char d3d_fourcc[5] = {0};
                    std::memcpy(d3d_fourcc, sdata + 76, 4);

                    uint16_t width  = read_u16(sdata + 80);
                    uint16_t height = read_u16(sdata + 82);
                    uint8_t depth   = sdata[84];
                    uint32_t dsize  = read_u32(sdata + 88);

                    const uint8_t* pixel_src = sdata + 92;

                    if (width > 0 && height > 0 && dsize > 0 && (pixel_src + dsize <= data + size)) {
                        auto* raster = rw::RwRasterCreate(width, height, 32, 0);
                        if (raster) {
                            uint8_t* locked_pixels = rw::RwRasterLock(raster, 0, 0);
                            if (locked_pixels) {
                                auto* dst32 = reinterpret_cast<uint32_t*>(locked_pixels);

                                if (std::strcmp(d3d_fourcc, "DXT1") == 0) {
                                    decode_dxt1(pixel_src, width, height, dst32);
                                } else if (std::strcmp(d3d_fourcc, "DXT3") == 0) {
                                    decode_dxt3(pixel_src, width, height, dst32);
                                } else if (std::strcmp(d3d_fourcc, "DXT5") == 0) {
                                    decode_dxt5(pixel_src, width, height, dst32);
                                } else if (depth == 32) {
                                    std::memcpy(dst32, pixel_src, static_cast<size_t>(width) * height * 4);
                                } else if (depth == 16 && d3d_fourcc[0] == '\0') {
                                    // 16-bit uncompressed (RGB 565)
                                    const auto* src16 = reinterpret_cast<const uint16_t*>(pixel_src);
                                    for (int p = 0; p < width * height; ++p) {
                                        uint16_t val = src16[p];
                                        uint32_t r = ((val >> 11) & 0x1F) * 255 / 31;
                                        uint32_t g = ((val >> 5) & 0x3F) * 255 / 63;
                                        uint32_t b = (val & 0x1F) * 255 / 31;
                                        dst32[p] = 0xFF000000u | (r << 16) | (g << 8) | b;
                                    }
                                } else if (depth == 8) {
                                    // 256 palette entries (1024 bytes) followed by width*height indices
                                    const auto* pal = reinterpret_cast<const uint32_t*>(pixel_src);
                                    const uint8_t* idx_bytes = pixel_src + 1024;
                                    for (int p = 0; p < width * height; ++p) {
                                        dst32[p] = pal[idx_bytes[p]];
                                    }
                                }

                                rw::RwRasterUnlock(raster);
                                auto* tex = rw::RwTextureCreate(raster);
                                if (tex) {
                                    std::strncpy(tex->name, tex_name.c_str(), sizeof(tex->name) - 1);
                                    tex->name[sizeof(tex->name) - 1] = '\0';
                                    if (tex->guest_addr && runtime::GuestMemory::instance().is_initialized()) {
                                        std::memcpy(runtime::GuestMemory::instance().translate(tex->guest_addr + 0x10), tex->name, 32);
                                    }
                                    result[tex_name] = tex;
                                }
                            }
                        }
                    }
                }
            }
        }

        pos = next_pos;
    }

    return result;
}

// ----------------------------------------------------------------------------
// TextureDictionary Implementation
// ----------------------------------------------------------------------------

TextureDictionary& TextureDictionary::instance() {
    static TextureDictionary s_instance;
    return s_instance;
}

std::string TextureDictionary::to_lower(std::string_view str) {
    std::string lower(str);
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return lower;
}

bool TextureDictionary::load_dictionary(const std::string& dict_name, const std::filesystem::path& path) {
    std::filesystem::path resolved_path = path;
    if (!std::filesystem::exists(resolved_path)) {
        auto vfs_opt = platform::VirtualFileSystem::instance().resolve(path.string());
        if (vfs_opt.has_value()) {
            resolved_path = *vfs_opt;
        } else {
            std::cerr << "[TextureDictionary] Could not resolve dictionary path: " << path << "\n";
            return false;
        }
    }

    auto textures = TxdLoader::load_from_file(resolved_path);
    if (textures.empty()) {
        std::cerr << "[TextureDictionary] Failed to load or empty dictionary: " << resolved_path << "\n";
        return false;
    }

    for (const auto& [tname, tex] : textures) {
        register_texture(tname, tex);
    }

    std::cout << "[TextureDictionary] Loaded dictionary '" << dict_name << "' ("
              << textures.size() << " textures) from " << resolved_path.filename().string() << ".\n";
    return true;
}

rw::RwTexture* TextureDictionary::find_texture(const std::string& name) const {
    if (name.empty()) return nullptr;
    std::string key = to_lower(name);
    auto it = m_textures.find(key);
    if (it != m_textures.end()) {
        return it->second;
    }
    return nullptr;
}

void TextureDictionary::register_texture(const std::string& name, rw::RwTexture* tex) {
    if (!tex || name.empty()) return;
    std::string key = to_lower(name);
    m_textures[key] = tex;
}

void TextureDictionary::clear() {
    m_textures.clear();
}

} // namespace pcrecomp::assets
