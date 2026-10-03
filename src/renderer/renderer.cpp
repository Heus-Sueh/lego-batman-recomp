#include "renderer.h"
#include "../platform/platform.h"
#include "../rw/rw_core.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <thread>


namespace pcrecomp::renderer {

Renderer& Renderer::instance() {
    static Renderer s_instance;
    return s_instance;
}

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize(RendererTier initial_tier) {
    m_tier = initial_tier;
    m_sim_tick_duration_us = 1000000 / m_simulation_fps;
    m_last_pacing_time = std::chrono::steady_clock::now();
    m_first_frame_time = m_last_pacing_time;
    m_last_present_time = m_last_pacing_time;
    m_sim_accumulator_us = 0;
    m_frame_count = 0;
    m_presented_count = 0;
    m_sim_ticks = 0;
    m_im2d_draws = 0;
    m_im3d_draws = 0;
    m_last_frame_ms = 0.0;
    m_splash_rendered = false;
    m_active_texture = nullptr;
    m_raster_textures.clear();
    set_perspective_projection(65.0f, 800.0f / 600.0f, 0.5f, 1500.0f);
    set_view_matrix(0.0f, -5.0f, 2.0f, 0.0f, 0.0f, 1.0f);
    m_initialized = true;

    std::cout << "[Renderer] Initialized at Tier " << static_cast<int>(m_tier)
              << " (Decoupled simulation rate: " << m_simulation_fps
              << " FPS, tick duration: " << m_sim_tick_duration_us << " us)\n";
    return true;
}

void Renderer::shutdown() {
    if (m_initialized) {
        for (auto& [raster, tex] : m_raster_textures) {
            destroy_native_texture(tex);
        }
        m_raster_textures.clear();

        std::cout << "[Renderer] Frame presentation stats:\n"
                  << "  Total frame passes:     " << m_frame_count << "\n"
                  << "  Swapchain presentations: " << m_presented_count << "\n"
                  << "  Simulation ticks:        " << m_sim_ticks << "\n"
                  << "  2D primitive draws:      " << m_im2d_draws << "\n"
                  << "  3D primitive draws:      " << m_im3d_draws << "\n"
                  << "  Average presentation FPS:" << average_fps() << "\n";
        m_initialized = false;
        std::cout << "[Renderer] Shutdown complete.\n";
    }
}

void Renderer::set_simulation_target_fps(uint32_t fps) noexcept {
    if (fps == 0) fps = 30;
    m_simulation_fps = fps;
    m_sim_tick_duration_us = 1000000 / fps;
}

uint32_t Renderer::consume_simulation_ticks() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_last_pacing_time).count();
    m_last_pacing_time = now;

    m_sim_accumulator_us += elapsed;

    uint32_t ticks = 0;
    while (m_sim_accumulator_us >= m_sim_tick_duration_us) {
        ++ticks;
        m_sim_accumulator_us -= m_sim_tick_duration_us;
    }

    // Clamp ticks to prevent spiral of death on system stalls
    return std::min(ticks, 4u);
}

void Renderer::begin_frame(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    platform::Platform::instance().clear_frame(r, g, b, a);
    ++m_frame_count;
}

void Renderer::end_frame() {
    // Finalize host frame render passes
}

void Renderer::present() {
    platform::Platform::instance().present_frame();
    ++m_presented_count;

    auto now = std::chrono::steady_clock::now();
    // Enforce display pacing (e.g. 60 FPS = 16666 us) when running with a visible host window
    if (m_host_target_fps > 0 && platform::Platform::instance().native_window_handle() != nullptr) {
        auto frame_interval = std::chrono::microseconds(1000000 / m_host_target_fps);
        auto target_time = m_last_present_time + frame_interval;
        if (target_time > now) {
            std::this_thread::sleep_until(target_time);
            now = std::chrono::steady_clock::now();
        }
    }

    auto delta_us = std::chrono::duration_cast<std::chrono::microseconds>(now - m_last_present_time).count();
    m_last_present_time = now;
    m_last_frame_ms = delta_us / 1000.0;
}

double Renderer::average_fps() const noexcept {
    if (m_presented_count == 0) return 0.0;
    auto now = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_first_frame_time).count();
    if (total_ms == 0) return 0.0;
    return (static_cast<double>(m_presented_count) * 1000.0) / static_cast<double>(total_ms);
}

// ----------------------------------------------------------------------------
// Tier 2 Resource Interception: Native Texture Cache
// ----------------------------------------------------------------------------

void* Renderer::create_native_texture(int width, int height) {
    SDL_Renderer* ren = static_cast<SDL_Renderer*>(platform::Platform::instance().native_renderer_handle());
    if (!ren) return nullptr;

    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!tex) {
        std::cerr << "[Renderer] SDL_CreateTexture failed: " << SDL_GetError() << "\n";
        return nullptr;
    }
    return tex;
}

void Renderer::update_native_texture(void* native_tex, int width, int height, const void* pixels, int pitch) {
    if (!native_tex || !pixels) return;
    int effective_pitch = pitch > 0 ? pitch : width * 4;
    SDL_UpdateTexture(static_cast<SDL_Texture*>(native_tex), nullptr, pixels, effective_pitch);
}

void Renderer::destroy_native_texture(void* native_tex) {
    if (native_tex) {
        SDL_DestroyTexture(static_cast<SDL_Texture*>(native_tex));
    }
}

void Renderer::bind_texture(void* native_tex) {
    m_active_texture = native_tex;
}

void Renderer::on_raster_created(const void* raster, int width, int height) {
    (void)raster;
    (void)width;
    (void)height;
}

void Renderer::on_raster_unlocked(const void* raster, int width, int height, const void* pixels, int pitch) {
    if (!raster || !pixels) return;

    void* tex = nullptr;
    auto it = m_raster_textures.find(raster);
    if (it != m_raster_textures.end()) {
        tex = it->second;
    } else {
        tex = create_native_texture(width, height);
        if (tex) {
            m_raster_textures[raster] = tex;
        }
    }

    if (tex) {
        update_native_texture(tex, width, height, pixels, pitch);
    }
}

void Renderer::on_raster_destroyed(const void* raster) {
    auto it = m_raster_textures.find(raster);
    if (it != m_raster_textures.end()) {
        destroy_native_texture(it->second);
        m_raster_textures.erase(it);
    }
}

void Renderer::register_raster_alias(const void* canonical_raster, const void* alias) {
    if (!canonical_raster || !alias) return;
    auto it = m_raster_textures.find(canonical_raster);
    if (it != m_raster_textures.end()) {
        m_raster_textures[alias] = it->second;
    }
}

void* Renderer::get_raster_native_texture(const void* raster) const {
    auto it = m_raster_textures.find(raster);
    if (it != m_raster_textures.end()) {
        return it->second;
    }
    return nullptr;
}

// ----------------------------------------------------------------------------
// 2D Immediate-Mode Geometry Submission (RwIm2D)
// ----------------------------------------------------------------------------

bool Renderer::render_im2d(uint32_t prim_type, const void* vertices, int num_vertices, const uint16_t* indices, int num_indices) {
    if (!vertices || num_vertices <= 0) return false;

    SDL_Renderer* ren = static_cast<SDL_Renderer*>(platform::Platform::instance().native_renderer_handle());
    const auto* rw_verts = static_cast<const rw::RwIm2DVertex*>(vertices);

    std::vector<SDL_Vertex> sdl_verts(num_vertices);
    for (int i = 0; i < num_vertices; ++i) {
        sdl_verts[i].position.x = rw_verts[i].x;
        sdl_verts[i].position.y = rw_verts[i].y;

        float a = ((rw_verts[i].emissiveColor >> 24) & 0xFF) / 255.0f;
        float r = ((rw_verts[i].emissiveColor >> 16) & 0xFF) / 255.0f;
        float g = ((rw_verts[i].emissiveColor >> 8) & 0xFF) / 255.0f;
        float b = (rw_verts[i].emissiveColor & 0xFF) / 255.0f;

        sdl_verts[i].color = SDL_FColor{r, g, b, a};
        sdl_verts[i].tex_coord.x = rw_verts[i].u;
        sdl_verts[i].tex_coord.y = rw_verts[i].v;
    }

    std::vector<int> sdl_indices;
    if (prim_type == rw::rwPRIMTYPETRIFAN) {
        for (int i = 1; i < num_vertices - 1; ++i) {
            sdl_indices.push_back(0);
            sdl_indices.push_back(i);
            sdl_indices.push_back(i + 1);
        }
    } else if (prim_type == rw::rwPRIMTYPETRISTRIP) {
        for (int i = 0; i < num_vertices - 2; ++i) {
            if (i % 2 == 0) {
                sdl_indices.push_back(i);
                sdl_indices.push_back(i + 1);
                sdl_indices.push_back(i + 2);
            } else {
                sdl_indices.push_back(i + 1);
                sdl_indices.push_back(i);
                sdl_indices.push_back(i + 2);
            }
        }
    } else if (prim_type == rw::rwPRIMTYPETRILIST && indices && num_indices > 0) {
        sdl_indices.reserve(num_indices);
        for (int i = 0; i < num_indices; ++i) {
            sdl_indices.push_back(static_cast<int>(indices[i]));
        }
    }

    if (ren) {
        SDL_Texture* tex = static_cast<SDL_Texture*>(m_active_texture);
        SDL_RenderGeometry(
            ren,
            tex,
            sdl_verts.data(),
            static_cast<int>(sdl_verts.size()),
            sdl_indices.empty() ? nullptr : sdl_indices.data(),
            static_cast<int>(sdl_indices.size())
        );
    }

    ++m_im2d_draws;
    return true;
}

// ----------------------------------------------------------------------------
// Tier 3: 3D Camera Projection & Immediate Mode Geometry Submission (RwIm3D)
// ----------------------------------------------------------------------------

void Renderer::set_perspective_projection(float fov_deg, float aspect, float near_z, float far_z) {
    m_fov_deg = fov_deg > 1.0f ? fov_deg : 65.0f;
    m_aspect = aspect > 0.01f ? aspect : (800.0f / 600.0f);
    m_near_z = near_z > 0.001f ? near_z : 0.5f;
    m_far_z = far_z > m_near_z ? far_z : 1500.0f;

    float f = 1.0f / std::tan((m_fov_deg * 3.141592653589793f / 180.0f) * 0.5f);
    std::fill(std::begin(m_proj_matrix), std::end(m_proj_matrix), 0.0f);
    m_proj_matrix[0]  = f / m_aspect;
    m_proj_matrix[5]  = f;
    m_proj_matrix[10] = m_far_z / (m_far_z - m_near_z);
    m_proj_matrix[11] = 1.0f;
    m_proj_matrix[14] = -(m_far_z * m_near_z) / (m_far_z - m_near_z);
}

void Renderer::set_view_matrix(float eye_x, float eye_y, float eye_z, float target_x, float target_y, float target_z) {
    m_cam_eye[0] = eye_x;
    m_cam_eye[1] = eye_y;
    m_cam_eye[2] = eye_z;

    // Forward direction: target - eye
    float fx = target_x - eye_x;
    float fy = target_y - eye_y;
    float fz = target_z - eye_z;
    float f_len = std::sqrt(fx * fx + fy * fy + fz * fz);
    if (f_len > 1e-6f) {
        fx /= f_len;
        fy /= f_len;
        fz /= f_len;
    } else {
        fx = 0.0f; fy = 1.0f; fz = 0.0f;
    }
    m_cam_forward[0] = fx;
    m_cam_forward[1] = fy;
    m_cam_forward[2] = fz;

    // In GTA San Andreas Z is UP: up_world = (0, 0, 1)
    // Right = Forward x Up_world
    float rx = fy;
    float ry = -fx;
    float rz = 0.0f;
    float r_len = std::sqrt(rx * rx + ry * ry);
    if (r_len > 1e-6f) {
        rx /= r_len;
        ry /= r_len;
    } else {
        rx = 1.0f; ry = 0.0f; rz = 0.0f;
    }
    m_cam_right[0] = rx;
    m_cam_right[1] = ry;
    m_cam_right[2] = rz;

    // Up = Right x Forward
    float ux = ry * fz - rz * fy;
    float uy = rz * fx - rx * fz;
    float uz = rx * fy - ry * fx;
    m_cam_up[0] = ux;
    m_cam_up[1] = uy;
    m_cam_up[2] = uz;

    std::fill(std::begin(m_view_matrix), std::end(m_view_matrix), 0.0f);
    m_view_matrix[0]  = rx; m_view_matrix[1]  = ux; m_view_matrix[2]  = fx;
    m_view_matrix[4]  = ry; m_view_matrix[5]  = uy; m_view_matrix[6]  = fy;
    m_view_matrix[8]  = rz; m_view_matrix[9]  = uz; m_view_matrix[10] = fz;
    m_view_matrix[12] = -(rx * eye_x + ry * eye_y + rz * eye_z);
    m_view_matrix[13] = -(ux * eye_x + uy * eye_y + uz * eye_z);
    m_view_matrix[14] = -(fx * eye_x + fy * eye_y + fz * eye_z);
    m_view_matrix[15] = 1.0f;
}

bool Renderer::render_im3d(uint32_t prim_type, const void* vertices, int num_vertices, const uint16_t* indices, int num_indices) {
    if (!vertices || num_vertices <= 0) return false;

    SDL_Renderer* ren = static_cast<SDL_Renderer*>(platform::Platform::instance().native_renderer_handle());
    const auto* rw_verts = static_cast<const rw::RwIm3DVertex*>(vertices);

    int win_w = platform::Platform::instance().window_width();
    int win_h = platform::Platform::instance().window_height();
    if (win_w <= 0) win_w = 800;
    if (win_h <= 0) win_h = 600;
    float fw = static_cast<float>(win_w);
    float fh = static_cast<float>(win_h);

    float fov_half_rad = (m_fov_deg * 3.141592653589793f / 180.0f) * 0.5f;
    float tan_fov = std::tan(fov_half_rad);
    float aspect = fw / fh;

    // Sunlight vector (San Andreas warm sun direction)
    constexpr float sun_x = 0.408248f;
    constexpr float sun_y = 0.408248f;
    constexpr float sun_z = 0.816497f;

    struct CamVertex {
        float cx, cy, cz;
        float u, v;
        SDL_FColor color;
    };

    auto transform_vertex = [&](const rw::RwIm3DVertex& v) -> CamVertex {
        float dx = v.objVertex.x - m_cam_eye[0];
        float dy = v.objVertex.y - m_cam_eye[1];
        float dz = v.objVertex.z - m_cam_eye[2];

        float cx = dx * m_cam_right[0]   + dy * m_cam_right[1]   + dz * m_cam_right[2];
        float cy = dx * m_cam_up[0]      + dy * m_cam_up[1]      + dz * m_cam_up[2];
        float cz = dx * m_cam_forward[0] + dy * m_cam_forward[1] + dz * m_cam_forward[2];

        // Normal lighting
        float nx = v.objNormal.x;
        float ny = v.objNormal.y;
        float nz = v.objNormal.z;
        float dot = nx * sun_x + ny * sun_y + nz * sun_z;
        if (dot < 0.0f) dot = 0.0f;
        float light_intensity = 0.45f + 0.55f * dot;

        float a = ((v.color >> 24) & 0xFF) / 255.0f;
        float r = (((v.color >> 16) & 0xFF) / 255.0f) * light_intensity;
        float g = (((v.color >> 8) & 0xFF) / 255.0f) * light_intensity;
        float b = ((v.color & 0xFF) / 255.0f) * light_intensity;

        return CamVertex{ cx, cy, cz, v.u, v.v, SDL_FColor{ std::min(r, 1.0f), std::min(g, 1.0f), std::min(b, 1.0f), a } };
    };

    struct Tri {
        CamVertex v[3];
        float avg_depth;
    };

    std::vector<Tri> triangles;

    auto add_triangle = [&](const CamVertex& v0, const CamVertex& v1, const CamVertex& v2) {
        float near_z = m_near_z;

        bool b0 = v0.cz < near_z;
        bool b1 = v1.cz < near_z;
        bool b2 = v2.cz < near_z;

        if (b0 && b1 && b2) {
            return;
        }

        auto lerp_vert = [&](const CamVertex& a, const CamVertex& b, float t) -> CamVertex {
            return CamVertex{
                a.cx + (b.cx - a.cx) * t,
                a.cy + (b.cy - a.cy) * t,
                near_z,
                a.u + (b.u - a.u) * t,
                a.v + (b.v - a.v) * t,
                SDL_FColor{
                    a.color.r + (b.color.r - a.color.r) * t,
                    a.color.g + (b.color.g - a.color.g) * t,
                    a.color.b + (b.color.b - a.color.b) * t,
                    a.color.a + (b.color.a - a.color.a) * t,
                }
            };
        };

        if (!b0 && !b1 && !b2) {
            float depth = (v0.cz + v1.cz + v2.cz) / 3.0f;
            triangles.push_back({ { v0, v1, v2 }, depth });
        } else if (b0 && !b1 && !b2) {
            float t01 = (near_z - v0.cz) / (v1.cz - v0.cz);
            float t02 = (near_z - v0.cz) / (v2.cz - v0.cz);
            CamVertex v01 = lerp_vert(v0, v1, t01);
            CamVertex v02 = lerp_vert(v0, v2, t02);
            triangles.push_back({ { v01, v1, v2 }, (v01.cz + v1.cz + v2.cz) / 3.0f });
            triangles.push_back({ { v01, v2, v02 }, (v01.cz + v2.cz + v02.cz) / 3.0f });
        } else if (!b0 && b1 && !b2) {
            float t10 = (near_z - v1.cz) / (v0.cz - v1.cz);
            float t12 = (near_z - v1.cz) / (v2.cz - v1.cz);
            CamVertex v10 = lerp_vert(v1, v0, t10);
            CamVertex v12 = lerp_vert(v1, v2, t12);
            triangles.push_back({ { v0, v10, v2 }, (v0.cz + v10.cz + v2.cz) / 3.0f });
            triangles.push_back({ { v10, v12, v2 }, (v10.cz + v12.cz + v2.cz) / 3.0f });
        } else if (!b0 && !b1 && b2) {
            float t20 = (near_z - v2.cz) / (v0.cz - v2.cz);
            float t21 = (near_z - v2.cz) / (v1.cz - v2.cz);
            CamVertex v20 = lerp_vert(v2, v0, t20);
            CamVertex v21 = lerp_vert(v2, v1, t21);
            triangles.push_back({ { v0, v1, v20 }, (v0.cz + v1.cz + v20.cz) / 3.0f });
            triangles.push_back({ { v1, v21, v20 }, (v1.cz + v21.cz + v20.cz) / 3.0f });
        } else if (b0 && b1 && !b2) {
            float t02 = (near_z - v0.cz) / (v2.cz - v0.cz);
            float t12 = (near_z - v1.cz) / (v2.cz - v1.cz);
            CamVertex v02 = lerp_vert(v0, v2, t02);
            CamVertex v12 = lerp_vert(v1, v2, t12);
            triangles.push_back({ { v02, v12, v2 }, (v02.cz + v12.cz + v2.cz) / 3.0f });
        } else if (b0 && !b1 && b2) {
            float t01 = (near_z - v0.cz) / (v1.cz - v0.cz);
            float t21 = (near_z - v2.cz) / (v1.cz - v2.cz);
            CamVertex v01 = lerp_vert(v0, v1, t01);
            CamVertex v21 = lerp_vert(v2, v1, t21);
            triangles.push_back({ { v01, v1, v21 }, (v01.cz + v1.cz + v21.cz) / 3.0f });
        } else if (!b0 && b1 && b2) {
            float t10 = (near_z - v1.cz) / (v0.cz - v1.cz);
            float t20 = (near_z - v2.cz) / (v0.cz - v2.cz);
            CamVertex v10 = lerp_vert(v1, v0, t10);
            CamVertex v20 = lerp_vert(v2, v0, t20);
            triangles.push_back({ { v0, v10, v20 }, (v0.cz + v10.cz + v20.cz) / 3.0f });
        }
    };

    if (prim_type == rw::rwPRIMTYPETRILIST) {
        if (indices && num_indices > 0) {
            for (int i = 0; i + 2 < num_indices; i += 3) {
                auto cv0 = transform_vertex(rw_verts[indices[i]]);
                auto cv1 = transform_vertex(rw_verts[indices[i + 1]]);
                auto cv2 = transform_vertex(rw_verts[indices[i + 2]]);
                add_triangle(cv0, cv1, cv2);
            }
        } else {
            for (int i = 0; i + 2 < num_vertices; i += 3) {
                auto cv0 = transform_vertex(rw_verts[i]);
                auto cv1 = transform_vertex(rw_verts[i + 1]);
                auto cv2 = transform_vertex(rw_verts[i + 2]);
                add_triangle(cv0, cv1, cv2);
            }
        }
    } else if (prim_type == rw::rwPRIMTYPETRISTRIP) {
        for (int i = 0; i + 2 < num_vertices; ++i) {
            auto cv0 = transform_vertex(rw_verts[i]);
            auto cv1 = transform_vertex(rw_verts[i + 1]);
            auto cv2 = transform_vertex(rw_verts[i + 2]);
            if (i % 2 == 0) {
                add_triangle(cv0, cv1, cv2);
            } else {
                add_triangle(cv1, cv0, cv2);
            }
        }
    } else if (prim_type == rw::rwPRIMTYPETRIFAN) {
        auto cv0 = transform_vertex(rw_verts[0]);
        for (int i = 1; i + 1 < num_vertices; ++i) {
            auto cv1 = transform_vertex(rw_verts[i]);
            auto cv2 = transform_vertex(rw_verts[i + 1]);
            add_triangle(cv0, cv1, cv2);
        }
    }

    if (triangles.empty()) {
        ++m_im3d_draws;
        return true;
    }

    // Sort triangles back-to-front (Painter's Algorithm for correct depth order)
    std::sort(triangles.begin(), triangles.end(), [](const Tri& a, const Tri& b) {
        return a.avg_depth > b.avg_depth;
    });

    std::vector<SDL_Vertex> sdl_verts;
    sdl_verts.reserve(triangles.size() * 3);

    for (const auto& tri : triangles) {
        for (int k = 0; k < 3; ++k) {
            const auto& cv = tri.v[k];
            float ndc_x = cv.cx / (cv.cz * aspect * tan_fov);
            float ndc_y = cv.cy / (cv.cz * tan_fov);

            float screen_x = (ndc_x + 1.0f) * 0.5f * fw;
            float screen_y = (1.0f - ndc_y) * 0.5f * fh;

            sdl_verts.push_back(SDL_Vertex{
                SDL_FPoint{ screen_x, screen_y },
                cv.color,
                SDL_FPoint{ cv.u, cv.v }
            });
        }
    }

    if (ren && !sdl_verts.empty()) {
        SDL_Texture* tex = static_cast<SDL_Texture*>(m_active_texture);
        SDL_RenderGeometry(
            ren,
            tex,
            sdl_verts.data(),
            static_cast<int>(sdl_verts.size()),
            nullptr,
            0
        );
    }

    ++m_im3d_draws;
    return true;
}

void Renderer::render_splash_screen() {
    int w = platform::Platform::instance().window_width();
    int h = platform::Platform::instance().window_height();
    if (w <= 0) w = 800;
    if (h <= 0) h = 600;

    // Prioritize authentic splash texture from models/txd/loadsc0.txd
    static void* s_splash_tex = nullptr;
    if (!s_splash_tex) {
        rw::RwRaster* splash_raster = rw::CLoadingScreen_GetSplashRaster();
        if (splash_raster) {
            s_splash_tex = get_raster_native_texture(splash_raster);
        }

        if (!s_splash_tex) {
            constexpr int tex_w = 256;
            constexpr int tex_h = 256;
            std::vector<uint32_t> pixels(tex_w * tex_h);

            // Fallback stylish San Andreas sunset gradient (Deep Orange / Purple / Yellow)
            for (int y = 0; y < tex_h; ++y) {
                float t = static_cast<float>(y) / static_cast<float>(tex_h);
                uint8_t r = static_cast<uint8_t>((1.0f - t) * 220 + t * 40);
                uint8_t g = static_cast<uint8_t>((1.0f - t) * 110 + t * 15);
                uint8_t b = static_cast<uint8_t>((1.0f - t) * 30 + t * 90);
                uint32_t color = (0xFFu << 24) | (r << 16) | (g << 8) | b;
                for (int x = 0; x < tex_w; ++x) {
                    pixels[y * tex_w + x] = color;
                }
            }

            s_splash_tex = create_native_texture(tex_w, tex_h);
            if (s_splash_tex) {
                update_native_texture(s_splash_tex, tex_w, tex_h, pixels.data(), tex_w * 4);
            }
        }
    }

    bind_texture(s_splash_tex);

    // Fullscreen 2D quad (TRIANGLEFAN: top-left, top-right, bottom-right, bottom-left)
    rw::RwIm2DVertex quad[4] = {
        { 0.0f,               0.0f,               0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f },
        { static_cast<float>(w), 0.0f,            0.0f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f },
        { static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f, 0xFFFFFFFF, 1.0f, 1.0f },
        { 0.0f,               static_cast<float>(h), 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 1.0f }
    };

    render_im2d(rw::rwPRIMTYPETRIFAN, quad, 4, nullptr, 0);
    bind_texture(nullptr);

    if (!m_splash_rendered) {
        std::cout << "[Renderer] Rendered native splash screen quad (Level 5 Visual reached).\n";
        m_splash_rendered = true;
    }
}

} // namespace pcrecomp::renderer
