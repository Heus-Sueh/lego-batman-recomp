#include "platform.h"
#include "../runtime/guest_memory.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace pcrecomp::platform {

namespace {

// Maps SDL scancodes to Win32 Virtual-Key codes (VK_*)
uint32_t sdl_scancode_to_win32_vk(SDL_Scancode sc) {
    switch (sc) {
        // Alphanumeric
        case SDL_SCANCODE_A: return 'A';
        case SDL_SCANCODE_B: return 'B';
        case SDL_SCANCODE_C: return 'C';
        case SDL_SCANCODE_D: return 'D';
        case SDL_SCANCODE_E: return 'E';
        case SDL_SCANCODE_F: return 'F';
        case SDL_SCANCODE_G: return 'G';
        case SDL_SCANCODE_H: return 'H';
        case SDL_SCANCODE_I: return 'I';
        case SDL_SCANCODE_J: return 'J';
        case SDL_SCANCODE_K: return 'K';
        case SDL_SCANCODE_L: return 'L';
        case SDL_SCANCODE_M: return 'M';
        case SDL_SCANCODE_N: return 'N';
        case SDL_SCANCODE_O: return 'O';
        case SDL_SCANCODE_P: return 'P';
        case SDL_SCANCODE_Q: return 'Q';
        case SDL_SCANCODE_R: return 'R';
        case SDL_SCANCODE_S: return 'S';
        case SDL_SCANCODE_T: return 'T';
        case SDL_SCANCODE_U: return 'U';
        case SDL_SCANCODE_V: return 'V';
        case SDL_SCANCODE_W: return 'W';
        case SDL_SCANCODE_X: return 'X';
        case SDL_SCANCODE_Y: return 'Y';
        case SDL_SCANCODE_Z: return 'Z';

        case SDL_SCANCODE_1: return '1';
        case SDL_SCANCODE_2: return '2';
        case SDL_SCANCODE_3: return '3';
        case SDL_SCANCODE_4: return '4';
        case SDL_SCANCODE_5: return '5';
        case SDL_SCANCODE_6: return '6';
        case SDL_SCANCODE_7: return '7';
        case SDL_SCANCODE_8: return '8';
        case SDL_SCANCODE_9: return '9';
        case SDL_SCANCODE_0: return '0';

        // Modifiers & System
        case SDL_SCANCODE_RETURN: return 0x0D; // VK_RETURN
        case SDL_SCANCODE_ESCAPE: return 0x1B; // VK_ESCAPE
        case SDL_SCANCODE_BACKSPACE: return 0x08; // VK_BACK
        case SDL_SCANCODE_TAB: return 0x09; // VK_TAB
        case SDL_SCANCODE_SPACE: return 0x20; // VK_SPACE
        case SDL_SCANCODE_CAPSLOCK: return 0x14; // VK_CAPITAL

        // Navigation
        case SDL_SCANCODE_RIGHT: return 0x27; // VK_RIGHT
        case SDL_SCANCODE_LEFT: return 0x25; // VK_LEFT
        case SDL_SCANCODE_DOWN: return 0x28; // VK_DOWN
        case SDL_SCANCODE_UP: return 0x26; // VK_UP
        case SDL_SCANCODE_PAGEUP: return 0x21; // VK_PRIOR
        case SDL_SCANCODE_PAGEDOWN: return 0x22; // VK_NEXT
        case SDL_SCANCODE_HOME: return 0x24; // VK_HOME
        case SDL_SCANCODE_END: return 0x23; // VK_END
        case SDL_SCANCODE_INSERT: return 0x2D; // VK_INSERT
        case SDL_SCANCODE_DELETE: return 0x2E; // VK_DELETE

        // Function keys
        case SDL_SCANCODE_F1: return 0x70;
        case SDL_SCANCODE_F2: return 0x71;
        case SDL_SCANCODE_F3: return 0x72;
        case SDL_SCANCODE_F4: return 0x73;
        case SDL_SCANCODE_F5: return 0x74;
        case SDL_SCANCODE_F6: return 0x75;
        case SDL_SCANCODE_F7: return 0x76;
        case SDL_SCANCODE_F8: return 0x77;
        case SDL_SCANCODE_F9: return 0x78;
        case SDL_SCANCODE_F10: return 0x79;
        case SDL_SCANCODE_F11: return 0x7A;
        case SDL_SCANCODE_F12: return 0x7B;

        // Modifiers
        case SDL_SCANCODE_LSHIFT: return 0xA0; // VK_LSHIFT
        case SDL_SCANCODE_RSHIFT: return 0xA1; // VK_RSHIFT
        case SDL_SCANCODE_LCTRL: return 0xA2; // VK_LCONTROL
        case SDL_SCANCODE_RCTRL: return 0xA3; // VK_RCONTROL
        case SDL_SCANCODE_LALT: return 0xA4; // VK_LMENU
        case SDL_SCANCODE_RALT: return 0xA5; // VK_RMENU

        default: return 0;
    }
}

} // namespace

Platform& Platform::instance() {
    static Platform s_instance;
    return s_instance;
}

Platform::Platform() = default;

Platform::~Platform() {
    shutdown();
}

bool Platform::initialize(const WindowConfig& config) {
    if (m_running) return true;

    m_config = config;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        std::cerr << "[Platform] SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_WindowFlags flags = 0;
    if (config.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    if (config.fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    SDL_Window* window = SDL_CreateWindow(
        config.title.c_str(),
        config.width,
        config.height,
        flags
    );

    if (!window) {
        std::cerr << "[Platform] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return false;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "[Platform] SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    if (config.letterbox) {
        SDL_SetRenderLogicalPresentation(renderer, config.width, config.height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }

    SDL_SetRenderVSync(renderer, config.vsync ? 1 : 0);

    m_window = window;
    m_renderer = renderer;
    m_running = true;

    // Check for already connected gamepads
    int count = 0;
    SDL_JoystickID* joysticks = SDL_GetGamepads(&count);
    if (joysticks && count > 0) {
        m_gamepad = SDL_OpenGamepad(joysticks[0]);
        if (m_gamepad) {
            std::cout << "[Platform] Discovered and opened gamepad: " << SDL_GetGamepadName(static_cast<SDL_Gamepad*>(m_gamepad)) << "\n";
        }
        SDL_free(joysticks);
    }

    std::cout << "[Platform] SDL3 initialized successfully. Native window ("
              << config.width << "x" << config.height << ") and hardware presenter created.\n";
    return true;
}

void Platform::shutdown() {
    if (m_gamepad) {
        SDL_CloseGamepad(static_cast<SDL_Gamepad*>(m_gamepad));
        m_gamepad = nullptr;
    }

    if (m_renderer) {
        SDL_DestroyRenderer(static_cast<SDL_Renderer*>(m_renderer));
        m_renderer = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(static_cast<SDL_Window*>(m_window));
        m_window = nullptr;
    }

    if (m_running) {
        SDL_Quit();
        m_running = false;
        std::cout << "[Platform] SDL3 shutdown complete.\n";
    }
}

bool Platform::poll_events() {
    if (!m_running) return false;
    if (!m_window) return true; // Headless mode safe return

    // Reset relative mouse deltas at each poll tick
    m_mouse_dx = 0;
    m_mouse_dy = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT: {
                m_running = false;
                return false;
            }

            case SDL_EVENT_KEY_DOWN: {
                uint32_t sc = static_cast<uint32_t>(event.key.scancode);
                if (sc < m_scancodes.size()) {
                    m_scancodes[sc] = true;
                }
                uint32_t vk = sdl_scancode_to_win32_vk(event.key.scancode);
                if (vk > 0 && vk < m_vk_keys.size()) {
                    m_vk_keys[vk] = true;
                    // CPad::TempKeyState (0x00B731A8, array of 256 16-bit words)
#ifdef RECOMP_LEGACY_GAME_HOOKS
                    runtime::GuestMemory::instance().write16(0x00B731A8 + vk * 2, 255);
#endif
                }
                if (sc < 256) {
                    // CPad::NewKeyState (0x00B73458, array of 256 bytes)
#ifdef RECOMP_LEGACY_GAME_HOOKS
                    runtime::GuestMemory::instance().write8(0x00B73458 + sc, 0x80);
#endif
                }
                break;
            }

            case SDL_EVENT_KEY_UP: {
                uint32_t sc = static_cast<uint32_t>(event.key.scancode);
                if (sc < m_scancodes.size()) {
                    m_scancodes[sc] = false;
                }
                uint32_t vk = sdl_scancode_to_win32_vk(event.key.scancode);
                if (vk > 0 && vk < m_vk_keys.size()) {
                    m_vk_keys[vk] = false;
#ifdef RECOMP_LEGACY_GAME_HOOKS
                    runtime::GuestMemory::instance().write16(0x00B731A8 + vk * 2, 0);
#endif
                }
                if (sc < 256) {
#ifdef RECOMP_LEGACY_GAME_HOOKS
                    runtime::GuestMemory::instance().write8(0x00B73458 + sc, 0);
#endif
                }
                break;
            }

            case SDL_EVENT_MOUSE_MOTION: {
                m_mouse_x = static_cast<int>(event.motion.x);
                m_mouse_y = static_cast<int>(event.motion.y);
                m_mouse_dx += static_cast<int>(event.motion.xrel);
                m_mouse_dy += static_cast<int>(event.motion.yrel);
                // Synchronize mouse coords to CMenuManager (0x00BA6748)
#ifdef RECOMP_LEGACY_GAME_HOOKS
                runtime::GuestMemory::instance().write32(0x00BA6748 + 0x64, static_cast<uint32_t>(m_mouse_x));
#endif
#ifdef RECOMP_LEGACY_GAME_HOOKS
                runtime::GuestMemory::instance().write32(0x00BA6748 + 0x68, static_cast<uint32_t>(m_mouse_y));
#endif
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_mouse_buttons |= (1u << 0);
#ifdef RECOMP_LEGACY_GAME_HOOKS
                    runtime::GuestMemory::instance().write8(0x00BA6748 + 0x6C, 1);
#endif
                }
                if (event.button.button == SDL_BUTTON_MIDDLE) m_mouse_buttons |= (1u << 1);
                if (event.button.button == SDL_BUTTON_RIGHT)  m_mouse_buttons |= (1u << 2);
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_UP: {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_mouse_buttons &= ~(1u << 0);
#ifdef RECOMP_LEGACY_GAME_HOOKS
                    runtime::GuestMemory::instance().write8(0x00BA6748 + 0x6C, 0);
#endif
                }
                if (event.button.button == SDL_BUTTON_MIDDLE) m_mouse_buttons &= ~(1u << 1);
                if (event.button.button == SDL_BUTTON_RIGHT)  m_mouse_buttons &= ~(1u << 2);
                break;
            }

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
                uint8_t btn = event.gbutton.button;
                uint32_t vk = 0;
                if (btn == SDL_GAMEPAD_BUTTON_DPAD_UP)    vk = 0x26; // VK_UP
                if (btn == SDL_GAMEPAD_BUTTON_DPAD_DOWN)  vk = 0x28; // VK_DOWN
                if (btn == SDL_GAMEPAD_BUTTON_DPAD_LEFT)  vk = 0x25; // VK_LEFT
                if (btn == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) vk = 0x27; // VK_RIGHT
                if (btn == SDL_GAMEPAD_BUTTON_SOUTH)      vk = 0x0D; // VK_RETURN
                if (btn == SDL_GAMEPAD_BUTTON_EAST)       vk = 0x1B; // VK_ESCAPE
                if (btn == SDL_GAMEPAD_BUTTON_START)      vk = 0x1B; // VK_ESCAPE
                if (vk > 0) {
                    m_vk_keys[vk] = true;
#ifdef RECOMP_LEGACY_GAME_HOOKS
                    runtime::GuestMemory::instance().write16(0x00B731A8 + vk * 2, 255);
#endif
                }
                break;
            }

            case SDL_EVENT_GAMEPAD_BUTTON_UP: {
                uint8_t btn = event.gbutton.button;
                uint32_t vk = 0;
                if (btn == SDL_GAMEPAD_BUTTON_DPAD_UP)    vk = 0x26; // VK_UP
                if (btn == SDL_GAMEPAD_BUTTON_DPAD_DOWN)  vk = 0x28; // VK_DOWN
                if (btn == SDL_GAMEPAD_BUTTON_DPAD_LEFT)  vk = 0x25; // VK_LEFT
                if (btn == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) vk = 0x27; // VK_RIGHT
                if (btn == SDL_GAMEPAD_BUTTON_SOUTH)      vk = 0x0D; // VK_RETURN
                if (btn == SDL_GAMEPAD_BUTTON_EAST)       vk = 0x1B; // VK_ESCAPE
                if (btn == SDL_GAMEPAD_BUTTON_START)      vk = 0x1B; // VK_ESCAPE
                if (vk > 0) {
                    m_vk_keys[vk] = false;
#ifdef RECOMP_LEGACY_GAME_HOOKS
                    runtime::GuestMemory::instance().write16(0x00B731A8 + vk * 2, 0);
#endif
                }
                break;
            }

            case SDL_EVENT_GAMEPAD_ADDED: {
                if (!m_gamepad) {
                    m_gamepad = SDL_OpenGamepad(event.gdevice.which);
                    if (m_gamepad) {
                        std::cout << "[Platform] Gamepad connected: "
                                  << SDL_GetGamepadName(static_cast<SDL_Gamepad*>(m_gamepad)) << "\n";
                    }
                }
                break;
            }

            case SDL_EVENT_GAMEPAD_REMOVED: {
                if (m_gamepad) {
                    SDL_CloseGamepad(static_cast<SDL_Gamepad*>(m_gamepad));
                    m_gamepad = nullptr;
                    std::cout << "[Platform] Gamepad disconnected.\n";
                }
                break;
            }

            default:
                break;
        }
    }

    return m_running;
}

void Platform::clear_frame(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (m_renderer) {
        SDL_SetRenderDrawColor(static_cast<SDL_Renderer*>(m_renderer), r, g, b, a);
        SDL_RenderClear(static_cast<SDL_Renderer*>(m_renderer));
    }
}

void Platform::present_frame() {
    if (m_renderer) {
        SDL_RenderPresent(static_cast<SDL_Renderer*>(m_renderer));
    }
}

bool Platform::is_key_down(uint32_t vk_code) const noexcept {
    if (vk_code < m_vk_keys.size()) {
        return m_vk_keys[vk_code];
    }
    return false;
}

bool Platform::is_scancode_down(uint32_t scancode) const noexcept {
    if (scancode < m_scancodes.size()) {
        return m_scancodes[scancode];
    }
    return false;
}

void Platform::get_mouse_position(int& x, int& y) const noexcept {
    x = m_mouse_x;
    y = m_mouse_y;
}

void Platform::get_mouse_delta(int& dx, int& dy) noexcept {
    dx = m_mouse_dx;
    dy = m_mouse_dy;
}

bool Platform::is_mouse_button_down(int button) const noexcept {
    if (button < 1 || button > 3) return false;
    return (m_mouse_buttons & (1u << (button - 1))) != 0;
}

void Platform::set_relative_mouse_mode(bool enabled) {
    if (m_window) {
        SDL_SetWindowRelativeMouseMode(static_cast<SDL_Window*>(m_window), enabled);
    }
}

bool Platform::has_gamepad() const noexcept {
    return m_gamepad != nullptr;
}

bool Platform::is_gamepad_button_down(int button) const noexcept {
    if (!m_gamepad) return false;
    return SDL_GetGamepadButton(static_cast<SDL_Gamepad*>(m_gamepad), static_cast<SDL_GamepadButton>(button));
}

int16_t Platform::get_gamepad_axis(int axis) const noexcept {
    if (!m_gamepad) return 0;
    return SDL_GetGamepadAxis(static_cast<SDL_Gamepad*>(m_gamepad), static_cast<SDL_GamepadAxis>(axis));
}

uint64_t Platform::get_ticks_ms() const noexcept {
    return SDL_GetTicks();
}

uint64_t Platform::get_ticks_us() const noexcept {
    return SDL_GetTicksNS() / 1000;
}

void Platform::sleep_ms(uint32_t ms) const noexcept {
    SDL_Delay(ms);
}

void Platform::sleep_us(uint64_t us) const noexcept {
    SDL_DelayNS(us * 1000);
}

void Platform::set_game_root(const std::string& path) {
    m_game_root = path;
    std::cout << "[Platform] Game root configured: " << m_game_root << "\n";
}

} // namespace pcrecomp::platform
