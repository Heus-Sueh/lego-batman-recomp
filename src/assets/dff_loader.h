#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include "../rw/rw_core.h"

namespace pcrecomp::assets {

struct DffGeometry {
    std::vector<rw::RwIm3DVertex> vertices;
    std::vector<uint16_t> indices;
    rw::RwV3d bounding_center{0.0f, 0.0f, 0.0f};
    float bounding_radius{0.0f};
};

struct DffModel {
    std::string name;
    std::vector<DffGeometry> geometries;

    void render(float x, float y, float z, float heading = 0.0f, float scale = 1.0f, uint32_t override_color = 0) const;
    void render(const rw::RwMatrix* ltm = nullptr, uint32_t override_color = 0) const;
};

class DffLoader {
public:
    static DffModel load_from_file(const std::filesystem::path& path);
    static DffModel load_from_memory(const uint8_t* data, size_t size, const std::string& name = "");
};

} // namespace pcrecomp::assets
