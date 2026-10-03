#pragma once

#include <string>
#include <cstdint>
#include <array>

namespace pcrecomp::platform {

struct WindowConfig {
    int width{800};
    int height{600};
    std::string title{"PC Recomp"};
    bool fullscreen{false};
    bool vsync{true};
    bool resizable{true};
    bool letterbox{true};
};

class Platform {
public:
    static Platform& instance();

    bool initialize(const WindowConfig& config = {});
    void shutdown();

    [[nodiscard]] bool is_running() const noexcept { return m_running; }
    void set_running(bool r) noexcept { m_running = r; }
    void request_exit() noexcept { m_running = false; }

    bool poll_events();
    void clear_frame(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255);
    void present_frame();

    // Input state inspection
    [[nodiscard]] bool is_key_down(uint32_t vk_code) const noexcept;
    [[nodiscard]] bool is_scancode_down(uint32_t scancode) const noexcept;

    void get_mouse_position(int& x, int& y) const noexcept;
    void get_mouse_delta(int& dx, int& dy) noexcept;
    [[nodiscard]] bool is_mouse_button_down(int button) const noexcept;
    void set_relative_mouse_mode(bool enabled);

    [[nodiscard]] bool has_gamepad() const noexcept;
    [[nodiscard]] bool is_gamepad_button_down(int button) const noexcept;
    [[nodiscard]] int16_t get_gamepad_axis(int axis) const noexcept;

    // High-resolution timing
    [[nodiscard]] uint64_t get_ticks_ms() const noexcept;
    [[nodiscard]] uint64_t get_ticks_us() const noexcept;
    void sleep_ms(uint32_t ms) const noexcept;
    void sleep_us(uint64_t us) const noexcept;

    void set_game_root(const std::string& path);
    [[nodiscard]] const std::string& game_root() const noexcept { return m_game_root; }

    [[nodiscard]] void* native_window_handle() const noexcept { return m_window; }
    [[nodiscard]] void* native_renderer_handle() const noexcept { return m_renderer; }

    [[nodiscard]] int window_width() const noexcept { return m_config.width; }
    [[nodiscard]] int window_height() const noexcept { return m_config.height; }

private:
    Platform();
    ~Platform();

    WindowConfig m_config;
    bool         m_running{false};
    void*        m_window{nullptr};
    void*        m_renderer{nullptr};
    void*        m_gamepad{nullptr};
    std::string  m_game_root;

    // Keyboard state: Win32 Virtual Key map (256 keys) and raw scancodes (512)
    std::array<bool, 256> m_vk_keys{};
    std::array<bool, 512> m_scancodes{};

    // Mouse state
    int  m_mouse_x{0};
    int  m_mouse_y{0};
    int  m_mouse_dx{0};
    int  m_mouse_dy{0};
    uint32_t m_mouse_buttons{0};
};

} // namespace pcrecomp::platform
