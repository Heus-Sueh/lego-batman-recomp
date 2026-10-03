#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <unordered_map>

namespace pcrecomp::renderer {

enum class RendererTier {
    Tier0_OriginalSemantics,     // D3D9 / RW original calls
    Tier1_NativePresentation,    // Native swapchain presentation pacing
    Tier2_ResourceInterception,  // Textures, vertex buffers, render targets
    Tier3_DrawInterception,      // RW draw-call interception
    Tier4_NativeVulkan           // Full Vulkan backend
};

class Renderer {
public:
    static Renderer& instance();

    bool initialize(RendererTier initial_tier = RendererTier::Tier2_ResourceInterception);
    void shutdown();

    [[nodiscard]] RendererTier current_tier() const noexcept { return m_tier; }
    void set_tier(RendererTier tier) noexcept { m_tier = tier; }

    // Host frame presentation hooks
    void begin_frame(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255);
    void end_frame();
    void present();

    // Decoupled frame pacing: 30 FPS guest simulation vs display presentation
    void set_simulation_target_fps(uint32_t fps) noexcept;
    [[nodiscard]] uint32_t simulation_target_fps() const noexcept { return m_simulation_fps; }

    void set_host_target_fps(uint32_t fps) noexcept { m_host_target_fps = fps; }
    [[nodiscard]] uint32_t host_target_fps() const noexcept { return m_host_target_fps; }

    // Calculates how many 30 FPS simulation steps are pending based on elapsed real time
    uint32_t consume_simulation_ticks();
    void record_simulation_tick() noexcept { ++m_sim_ticks; }

    // Tier 2 Resource Interception: Native Texture Cache
    void* create_native_texture(int width, int height);
    void  update_native_texture(void* native_tex, int width, int height, const void* pixels, int pitch);
    void  destroy_native_texture(void* native_tex);
    void  bind_texture(void* native_tex);

    void  on_raster_created(const void* raster, int width, int height);
    void  on_raster_unlocked(const void* raster, int width, int height, const void* pixels, int pitch);
    void  on_raster_destroyed(const void* raster);
    void  register_raster_alias(const void* canonical_raster, const void* alias);
    void* get_raster_native_texture(const void* raster) const;

    // 2D Immediate Mode Geometry Submission (RwIm2D)
    bool render_im2d(uint32_t prim_type, const void* vertices, int num_vertices, const uint16_t* indices = nullptr, int num_indices = 0);

    // Tier 3: 3D Camera Projection & Immediate Mode Geometry Submission (RwIm3D)
    void set_perspective_projection(float fov_deg, float aspect, float near_z, float far_z);
    void set_view_matrix(float eye_x, float eye_y, float eye_z, float target_x, float target_y, float target_z);
    bool render_im3d(uint32_t prim_type, const void* vertices, int num_vertices, const uint16_t* indices = nullptr, int num_indices = 0);

    // Splash / Visual Presentation helper
    void render_splash_screen();
    [[nodiscard]] bool was_splash_rendered() const noexcept { return m_splash_rendered; }

    // Diagnostics & Metrics
    [[nodiscard]] uint64_t total_frames() const noexcept { return m_frame_count; }
    [[nodiscard]] uint64_t presented_frames() const noexcept { return m_presented_count; }
    [[nodiscard]] uint64_t simulation_ticks() const noexcept { return m_sim_ticks; }
    [[nodiscard]] uint64_t im2d_draw_calls() const noexcept { return m_im2d_draws; }
    [[nodiscard]] uint64_t im3d_draw_calls() const noexcept { return m_im3d_draws; }
    [[nodiscard]] size_t   active_textures_count() const noexcept { return m_raster_textures.size(); }
    [[nodiscard]] double   average_fps() const noexcept;
    [[nodiscard]] double   last_frame_time_ms() const noexcept { return m_last_frame_ms; }

private:
    Renderer();
    ~Renderer();

    RendererTier m_tier{RendererTier::Tier2_ResourceInterception};
    uint32_t     m_simulation_fps{30};
    uint64_t     m_sim_tick_duration_us{33333};
    uint32_t     m_host_target_fps{60};
    bool         m_initialized{false};

    // Pacing state
    std::chrono::steady_clock::time_point m_last_pacing_time;
    std::chrono::steady_clock::time_point m_first_frame_time;
    std::chrono::steady_clock::time_point m_last_present_time;
    uint64_t     m_sim_accumulator_us{0};

    // Statistics
    uint64_t     m_frame_count{0};
    uint64_t     m_presented_count{0};
    uint64_t     m_sim_ticks{0};
    uint64_t     m_im2d_draws{0};
    uint64_t     m_im3d_draws{0};
    double       m_last_frame_ms{0.0};
    bool         m_splash_rendered{false};

    // 3D Perspective Matrix & Camera State
    float        m_view_matrix[16]{};
    float        m_proj_matrix[16]{};
    float        m_cam_eye[3]{0.0f, 0.0f, 0.0f};
    float        m_cam_forward[3]{0.0f, 1.0f, 0.0f};
    float        m_cam_right[3]{1.0f, 0.0f, 0.0f};
    float        m_cam_up[3]{0.0f, 0.0f, 1.0f};
    float        m_fov_deg{70.0f};
    float        m_aspect{1.333333f};
    float        m_near_z{0.5f};
    float        m_far_z{1000.0f};

    // Native GPU Texture Cache
    void* m_active_texture{nullptr};
    std::unordered_map<const void*, void*> m_raster_textures;
};

} // namespace pcrecomp::renderer
