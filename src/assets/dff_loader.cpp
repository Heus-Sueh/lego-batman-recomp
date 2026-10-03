#include "dff_loader.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

namespace pcrecomp::assets {

namespace {

constexpr uint32_t rwID_STRUCT         = 0x01;
constexpr uint32_t rwID_EXTENSION      = 0x03;
constexpr uint32_t rwID_GEOMETRY       = 0x0F;
constexpr uint32_t rwID_CLUMP          = 0x10;
constexpr uint32_t rwID_GEOMETRYLIST   = 0x1A;
constexpr uint32_t rwID_BINMESHPLUGIN  = 0x050E;

inline uint16_t read_u16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

inline uint32_t read_u32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

inline float read_float(const uint8_t* p) {
    float f = 0.0f;
    std::memcpy(&f, p, 4);
    return f;
}

} // namespace

void DffModel::render(float x, float y, float z, float heading, float scale, uint32_t override_color) const {
    rw::RwMatrix mat;
    float cos_h = std::cos(heading);
    float sin_h = std::sin(heading);

    mat.right = {  cos_h * scale, sin_h * scale, 0.0f };
    mat.up    = { -sin_h * scale, cos_h * scale, 0.0f };
    mat.at    = { 0.0f,          0.0f,          scale };
    mat.pos   = { x, y, z };

    render(&mat, override_color);
}

void DffModel::render(const rw::RwMatrix* ltm, uint32_t override_color) const {
    for (const auto& geom : geometries) {
        if (geom.vertices.empty() || geom.indices.empty()) continue;

        if (override_color != 0) {
            auto verts = geom.vertices;
            for (auto& v : verts) {
                v.color = override_color;
            }
            rw::RwIm3DTransform(verts.data(), static_cast<uint32_t>(verts.size()), ltm, 0);
            rw::RwIm3DRenderIndexedPrimitive(
                rw::rwPRIMTYPETRILIST,
                geom.indices.data(),
                static_cast<int32_t>(geom.indices.size())
            );
            rw::RwIm3DEnd();
        } else {
            rw::RwIm3DTransform(
                const_cast<rw::RwIm3DVertex*>(geom.vertices.data()),
                static_cast<uint32_t>(geom.vertices.size()),
                ltm,
                0
            );
            rw::RwIm3DRenderIndexedPrimitive(
                rw::rwPRIMTYPETRILIST,
                geom.indices.data(),
                static_cast<int32_t>(geom.indices.size())
            );
            rw::RwIm3DEnd();
        }
    }
}

DffModel DffLoader::load_from_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[DffLoader] Could not open DFF file: " << path << "\n";
        return {};
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(file_size);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));

    return load_from_memory(buffer.data(), buffer.size(), path.stem().string());
}

DffModel DffLoader::load_from_memory(const uint8_t* data, size_t size, const std::string& name) {
    DffModel model;
    model.name = name;
    if (!data || size < 24) return model;

    uint32_t root_id = read_u32(data);
    uint32_t root_sz = read_u32(data + 4);
    if (root_id != rwID_CLUMP) {
        std::cerr << "[DffLoader] Not a valid RenderWare DFF Clump chunk: 0x" << std::hex << root_id << "\n";
        return model;
    }

    size_t pos = 12;
    size_t max_pos = std::min(size, static_cast<size_t>(12 + root_sz));

    while (pos + 12 <= max_pos) {
        uint32_t cid = read_u32(data + pos);
        uint32_t csz = read_u32(data + pos + 4);
        size_t next_chunk = pos + 12 + csz;

        if (cid == rwID_GEOMETRYLIST) {
            size_t gpos = pos + 12;
            if (gpos + 16 <= size) {
                uint32_t scid = read_u32(data + gpos);
                uint32_t scsz = read_u32(data + gpos + 4);
                if (scid == rwID_STRUCT) {
                    uint32_t num_geoms = read_u32(data + gpos + 12);
                    gpos += 12 + scsz;

                    for (uint32_t gi = 0; gi < num_geoms && gpos + 12 <= size; ++gi) {
                        uint32_t gcid = read_u32(data + gpos);
                        uint32_t gcsz = read_u32(data + gpos + 4);
                        size_t next_geom = gpos + 12 + gcsz;

                        if (gcid == rwID_GEOMETRY && gpos + 12 + 12 <= size) {
                            const uint8_t* gdata = data + gpos + 12;
                            size_t inner_pos = 0;

                            DffGeometry geom;

                            // Struct
                            uint32_t s_id = read_u32(gdata + inner_pos);
                            uint32_t s_sz = read_u32(gdata + inner_pos + 4);
                            if (s_id == rwID_STRUCT && inner_pos + 12 + s_sz <= gcsz) {
                                const uint8_t* sdata = gdata + inner_pos + 12;

                                uint16_t flags = read_u16(sdata);
                                uint32_t num_uvs = (flags >> 8) & 0xFF;
                                if (num_uvs == 0 && (flags & 0x0004)) num_uvs = 1;

                                uint32_t num_triangles = read_u32(sdata + 4);
                                uint32_t num_vertices  = read_u32(sdata + 8);

                                size_t offset = 16;
                                const uint8_t* colors_ptr = nullptr;
                                if (flags & 0x0008) { // Prelit
                                    colors_ptr = sdata + offset;
                                    offset += num_vertices * 4;
                                }

                                const uint8_t* uvs_ptr = nullptr;
                                if (flags & 0x0004) { // Textured
                                    uvs_ptr = sdata + offset;
                                    offset += num_vertices * 8 * num_uvs;
                                }

                                if (num_triangles > 0 && offset + num_triangles * 8 <= s_sz) {
                                    geom.indices.reserve(num_triangles * 3);
                                    for (uint32_t t = 0; t < num_triangles; ++t) {
                                        uint16_t v1 = read_u16(sdata + offset);
                                        uint16_t v0 = read_u16(sdata + offset + 2);
                                        uint16_t v2 = read_u16(sdata + offset + 6);
                                        offset += 8;

                                        if (v0 < num_vertices && v1 < num_vertices && v2 < num_vertices) {
                                            geom.indices.push_back(v0);
                                            geom.indices.push_back(v1);
                                            geom.indices.push_back(v2);
                                        }
                                    }
                                }

                                if (offset + 24 <= s_sz) {
                                    geom.bounding_center.x = read_float(sdata + offset);
                                    geom.bounding_center.y = read_float(sdata + offset + 4);
                                    geom.bounding_center.z = read_float(sdata + offset + 8);
                                    geom.bounding_radius   = read_float(sdata + offset + 12);
                                    offset += 16;

                                    uint32_t has_verts = read_u32(sdata + offset);
                                    uint32_t has_norms = read_u32(sdata + offset + 4);
                                    offset += 8;

                                    const uint8_t* verts_ptr = nullptr;
                                    if (has_verts && offset + num_vertices * 12 <= s_sz) {
                                        verts_ptr = sdata + offset;
                                        offset += num_vertices * 12;
                                    }

                                    const uint8_t* norms_ptr = nullptr;
                                    if (has_norms && offset + num_vertices * 12 <= s_sz) {
                                        norms_ptr = sdata + offset;
                                        offset += num_vertices * 12;
                                    }

                                    if (verts_ptr && num_vertices > 0) {
                                        geom.vertices.resize(num_vertices);
                                        for (uint32_t v = 0; v < num_vertices; ++v) {
                                            geom.vertices[v].objVertex.x = read_float(verts_ptr + v * 12);
                                            geom.vertices[v].objVertex.y = read_float(verts_ptr + v * 12 + 4);
                                            geom.vertices[v].objVertex.z = read_float(verts_ptr + v * 12 + 8);

                                            if (norms_ptr) {
                                                geom.vertices[v].objNormal.x = read_float(norms_ptr + v * 12);
                                                geom.vertices[v].objNormal.y = read_float(norms_ptr + v * 12 + 4);
                                                geom.vertices[v].objNormal.z = read_float(norms_ptr + v * 12 + 8);
                                            } else {
                                                geom.vertices[v].objNormal = { 0.0f, 0.0f, 1.0f };
                                            }

                                            if (colors_ptr) {
                                                geom.vertices[v].color = read_u32(colors_ptr + v * 4);
                                            } else {
                                                geom.vertices[v].color = 0xFFFFFFFFu;
                                            }

                                            if (uvs_ptr) {
                                                geom.vertices[v].u = read_float(uvs_ptr + v * 8 * num_uvs);
                                                geom.vertices[v].v = read_float(uvs_ptr + v * 8 * num_uvs + 4);
                                            }
                                        }
                                    }
                                }

                                inner_pos += 12 + s_sz;
                            }

                            // Extension chunks for BinMeshPlugin if indices was empty
                            while (inner_pos + 12 <= gcsz) {
                                uint32_t sub_cid = read_u32(gdata + inner_pos);
                                uint32_t sub_csz = read_u32(gdata + inner_pos + 4);
                                size_t next_sub = inner_pos + 12 + sub_csz;

                                if (sub_cid == rwID_EXTENSION) {
                                    size_t ext_pos = inner_pos + 12;
                                    while (ext_pos + 12 <= next_sub) {
                                        uint32_t eid = read_u32(gdata + ext_pos);
                                        uint32_t esz = read_u32(gdata + ext_pos + 4);
                                        if (eid == rwID_BINMESHPLUGIN && ext_pos + 12 + esz <= next_sub) {
                                            const uint8_t* bdata = gdata + ext_pos + 12;
                                            uint32_t mesh_flags = read_u32(bdata);
                                            uint32_t num_meshes = read_u32(bdata + 4);
                                            size_t boffset = 12;

                                            if (geom.indices.empty()) {
                                                for (uint32_t m = 0; m < num_meshes && boffset + 8 <= esz; ++m) {
                                                    uint32_t num_indices = read_u32(bdata + boffset);
                                                    boffset += 8; // skip num_indices + mat_idx
                                                    if (mesh_flags & 1) {
                                                        // Tristrip
                                                        if (num_indices >= 3 && boffset + num_indices * 2 <= esz) {
                                                            std::vector<uint16_t> strip(num_indices);
                                                            for (uint32_t si = 0; si < num_indices; ++si) {
                                                                strip[si] = read_u16(bdata + boffset + si * 2);
                                                            }
                                                            for (uint32_t si = 0; si + 2 < num_indices; ++si) {
                                                                if (strip[si] != strip[si + 1] && strip[si + 1] != strip[si + 2] && strip[si] != strip[si + 2]) {
                                                                    if (si % 2 == 0) {
                                                                        geom.indices.push_back(strip[si]);
                                                                        geom.indices.push_back(strip[si + 1]);
                                                                        geom.indices.push_back(strip[si + 2]);
                                                                    } else {
                                                                        geom.indices.push_back(strip[si + 1]);
                                                                        geom.indices.push_back(strip[si]);
                                                                        geom.indices.push_back(strip[si + 2]);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    } else {
                                                        // Trilist
                                                        if (boffset + num_indices * 2 <= esz) {
                                                            for (uint32_t si = 0; si < num_indices; ++si) {
                                                                geom.indices.push_back(read_u16(bdata + boffset + si * 2));
                                                            }
                                                        }
                                                    }
                                                    boffset += num_indices * 2;
                                                }
                                            }
                                        }
                                        ext_pos += 12 + esz;
                                    }
                                }

                                inner_pos = next_sub;
                            }

                            if (!geom.vertices.empty()) {
                                model.geometries.push_back(std::move(geom));
                            }
                        }

                        gpos = next_geom;
                    }
                }
            }
        }

        pos = next_chunk;
    }

    if (!model.geometries.empty()) {
        std::cout << "[DffLoader] Loaded model '" << model.name << "' ("
                  << model.geometries.size() << " geometries)\n";
    }
    return model;
}

} // namespace pcrecomp::assets
