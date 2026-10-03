#include "rw_core.h"
#include "../renderer/renderer.h"
#include "../platform/platform.h"
#include "../platform/vfs.h"
#include "../assets/txd_loader.h"
#include "../runtime/guest_memory.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <unordered_map>

namespace pcrecomp::rw {

namespace {
RwCamera s_defaultCamera{};
RwRaster* s_splashRaster{nullptr};
RwTexture* s_splashTexture{nullptr};
std::vector<RwIm3DVertex> s_im3d_transformed;
bool s_im3d_active{false};

std::unordered_map<uint32_t, RwTexture*> s_guest_to_texture;
std::unordered_map<RwTexture*, uint32_t> s_texture_to_guest;
std::unordered_map<uint32_t, RwRaster*> s_guest_to_raster;
std::unordered_map<RwRaster*, uint32_t> s_raster_to_guest;
}

bool initialize_renderware_subsystems() {
    std::cout << "[RenderWare] Interception layer initialized (Tier 2 Rasters & 2D Im2D enabled).\n";
    return true;
}

void shutdown_renderware_subsystems() {
    CLoadingScreen_Shutdown();
    assets::TextureDictionary::instance().clear();
    s_guest_to_texture.clear();
    s_texture_to_guest.clear();
    s_guest_to_raster.clear();
    s_raster_to_guest.clear();
    std::cout << "[RenderWare] Interception layer shutdown.\n";
}

void* RwCameraBeginUpdate(void* camera) {
    renderer::Renderer::instance().begin_frame();
    return camera ? camera : &s_defaultCamera;
}

void* RwCameraEndUpdate(void* camera) {
    renderer::Renderer::instance().end_frame();
    renderer::Renderer::instance().present();
    return camera ? camera : &s_defaultCamera;
}

void* RwCameraClear(void* camera, const RwRGBA* colour, uint32_t clearFlags) {
    (void)clearFlags;
    if (colour) {
        renderer::Renderer::instance().begin_frame(colour->r, colour->g, colour->b, colour->a);
    } else {
        renderer::Renderer::instance().begin_frame(0, 0, 0, 255);
    }
    return camera ? camera : &s_defaultCamera;
}

bool dispatch_rs_event(uint32_t event, void* param) {
    (void)param;
    switch (event) {
        case rsINIT:
            std::cout << "[RenderWare] Event rsINIT received.\n";
            return true;
        case rsREGISTERIMAGEFORMATS:
            std::cout << "[RenderWare] Event rsREGISTERIMAGEFORMATS received.\n";
            return true;
        case rsRWINIT:
            std::cout << "[RenderWare] Event rsRWINIT received.\n";
            return true;
        case rsIDLE: {
            // Authentic RenderWare camera frame lifecycle
            RwCameraBeginUpdate(&s_defaultCamera);
            RwRGBA black{0, 0, 0, 255};
            RwCameraClear(&s_defaultCamera, &black, 3);
            RwCameraEndUpdate(&s_defaultCamera);
            return true;
        }
        case rsRWTERMINATE:
            std::cout << "[RenderWare] Event rsRWTERMINATE received.\n";
            return true;
        case rsFRONTENDQUITREQUEST:
            platform::Platform::instance().request_exit();
            return true;
        default:
            return true;
    }
}

// ----------------------------------------------------------------------------
// Tier 2 Resource Interception: Rasters & Textures
// ----------------------------------------------------------------------------

RwRaster* RwRasterCreate(int32_t width, int32_t height, int32_t depth, int32_t flags) {
    if (width <= 0 || height <= 0) return nullptr;

    auto* raster = new RwRaster();
    raster->width = width;
    raster->height = height;
    raster->depth = (depth > 0) ? depth : 32;
    raster->stride = width * 4; // 32-bit ARGB
    raster->cFlags = static_cast<uint8_t>(flags & 0xFF);

    size_t byte_size = static_cast<size_t>(raster->stride) * raster->height;
    raster->cpPixels = new uint8_t[byte_size]();

    if (runtime::GuestMemory::instance().is_initialized()) {
        uint32_t gaddr = runtime::GuestMemory::instance().guest_alloc(64);
        if (gaddr) {
            raster->guest_addr = gaddr;
            s_guest_to_raster[gaddr] = raster;
            s_raster_to_guest[raster] = gaddr;
            runtime::GuestMemory::instance().write32(gaddr + 0x00, gaddr); // parent
            runtime::GuestMemory::instance().write32(gaddr + 0x0C, width);
            runtime::GuestMemory::instance().write32(gaddr + 0x10, height);
            runtime::GuestMemory::instance().write32(gaddr + 0x14, raster->depth);
            runtime::GuestMemory::instance().write32(gaddr + 0x18, raster->stride);
        }
    }

    renderer::Renderer::instance().on_raster_created(raster, width, height);
    if (raster->guest_addr) {
        renderer::Renderer::instance().register_raster_alias(raster, reinterpret_cast<void*>(static_cast<uintptr_t>(raster->guest_addr)));
    }
    return raster;
}

bool RwRasterDestroy(RwRaster* raster) {
    if (!raster) return false;

    renderer::Renderer::instance().on_raster_destroyed(raster);
    if (raster->guest_addr) {
        renderer::Renderer::instance().on_raster_destroyed(reinterpret_cast<void*>(static_cast<uintptr_t>(raster->guest_addr)));
        s_guest_to_raster.erase(raster->guest_addr);
        s_raster_to_guest.erase(raster);
    }

    delete[] raster->cpPixels;
    raster->cpPixels = nullptr;
    delete raster;
    return true;
}

uint8_t* RwRasterLock(RwRaster* raster, uint8_t level, int32_t lockMode) {
    (void)level;
    (void)lockMode;
    if (!raster) return nullptr;
    return raster->cpPixels;
}

RwRaster* RwRasterUnlock(RwRaster* raster) {
    if (!raster) return nullptr;

    renderer::Renderer::instance().on_raster_unlocked(
        raster,
        raster->width,
        raster->height,
        raster->cpPixels,
        raster->stride
    );
    if (raster->guest_addr) {
        renderer::Renderer::instance().register_raster_alias(raster, reinterpret_cast<void*>(static_cast<uintptr_t>(raster->guest_addr)));
    }
    return raster;
}

RwRaster* RwRasterShowRaster(RwRaster* raster, void* dev, uint32_t flags) {
    (void)dev;
    (void)flags;
    if (!raster) return nullptr;

    void* tex = renderer::Renderer::instance().get_raster_native_texture(raster);
    if (tex) {
        int w = platform::Platform::instance().window_width();
        int h = platform::Platform::instance().window_height();
        if (w <= 0) w = 800;
        if (h <= 0) h = 600;

        renderer::Renderer::instance().bind_texture(tex);
        RwIm2DVertex quad[4] = {
            { 0.0f,               0.0f,               0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 0.0f },
            { static_cast<float>(w), 0.0f,            0.0f, 1.0f, 0xFFFFFFFF, 1.0f, 0.0f },
            { static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f, 0xFFFFFFFF, 1.0f, 1.0f },
            { 0.0f,               static_cast<float>(h), 0.0f, 1.0f, 0xFFFFFFFF, 0.0f, 1.0f }
        };
        renderer::Renderer::instance().render_im2d(rwPRIMTYPETRIFAN, quad, 4, nullptr, 0);
        renderer::Renderer::instance().bind_texture(nullptr);
    }

    return raster;
}

RwTexture* RwTextureCreate(RwRaster* raster) {
    auto* texture = new RwTexture();
    texture->raster = raster;
    texture->refCount = 1;

    if (runtime::GuestMemory::instance().is_initialized()) {
        uint32_t gaddr = runtime::GuestMemory::instance().guest_alloc(96);
        if (gaddr) {
            texture->guest_addr = gaddr;
            s_guest_to_texture[gaddr] = texture;
            s_texture_to_guest[texture] = gaddr;
            uint32_t r_gaddr = raster ? raster->guest_addr : 0;
            runtime::GuestMemory::instance().write32(gaddr + 0x00, r_gaddr); // offset 0: raster!
            runtime::GuestMemory::instance().write32(gaddr + 0x54, 1);       // refCount
        }
    }
    return texture;
}

bool RwTextureDestroy(RwTexture* texture) {
    if (!texture) return false;

    uintptr_t uaddr = reinterpret_cast<uintptr_t>(texture);
    if (uaddr < 0x10000) return false;

    // Check if passed a guest address
    if (uaddr < 0x100000000ull) {
        auto it = s_guest_to_texture.find(static_cast<uint32_t>(uaddr));
        if (it != s_guest_to_texture.end()) {
            texture = it->second;
        } else {
            // Unmapped guest address, cannot safely dereference as host pointer
            return false;
        }
    }

    if (!texture) return false;

    --texture->refCount;
    if (texture->refCount <= 0) {
        if (texture->guest_addr) {
            s_guest_to_texture.erase(texture->guest_addr);
            s_texture_to_guest.erase(texture);
        }
        if (texture->raster) {
            RwRasterDestroy(texture->raster);
            texture->raster = nullptr;
        }
        delete texture;
    }
    return true;
}

// ----------------------------------------------------------------------------
// 2D Immediate Mode Geometry Submission (RwIm2D)
// ----------------------------------------------------------------------------

bool RwIm2DRenderPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, int32_t numVertices) {
    return renderer::Renderer::instance().render_im2d(primType, vertices, numVertices, nullptr, 0);
}

bool RwIm2DRenderIndexedPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, int32_t numVertices, uint16_t* indices, int32_t numIndices) {
    return renderer::Renderer::instance().render_im2d(primType, vertices, numVertices, indices, numIndices);
}

// ----------------------------------------------------------------------------
// 3D Immediate Mode Geometry Submission (RwIm3D)
// ----------------------------------------------------------------------------

void* RwIm3DTransform(RwIm3DVertex* pVerts, uint32_t numVerts, const RwMatrix* ltm, uint32_t flags) {
    (void)flags;
    if (!pVerts || numVerts == 0) return nullptr;

    s_im3d_transformed.resize(numVerts);
    if (ltm) {
        for (uint32_t i = 0; i < numVerts; ++i) {
            s_im3d_transformed[i] = pVerts[i];
            const auto& v = pVerts[i].objVertex;
            s_im3d_transformed[i].objVertex.x = ltm->pos.x + v.x * ltm->right.x + v.y * ltm->up.x + v.z * ltm->at.x;
            s_im3d_transformed[i].objVertex.y = ltm->pos.y + v.x * ltm->right.y + v.y * ltm->up.y + v.z * ltm->at.y;
            s_im3d_transformed[i].objVertex.z = ltm->pos.z + v.x * ltm->right.z + v.y * ltm->up.z + v.z * ltm->at.z;

            const auto& n = pVerts[i].objNormal;
            s_im3d_transformed[i].objNormal.x = n.x * ltm->right.x + n.y * ltm->up.x + n.z * ltm->at.x;
            s_im3d_transformed[i].objNormal.y = n.x * ltm->right.y + n.y * ltm->up.y + n.z * ltm->at.y;
            s_im3d_transformed[i].objNormal.z = n.x * ltm->right.z + n.y * ltm->up.z + n.z * ltm->at.z;
        }
    } else {
        std::memcpy(s_im3d_transformed.data(), pVerts, numVerts * sizeof(RwIm3DVertex));
    }
    s_im3d_active = true;
    return s_im3d_transformed.data();
}

bool RwIm3DRenderPrimitive(RwPrimitiveType primType) {
    if (!s_im3d_active || s_im3d_transformed.empty()) return false;
    return renderer::Renderer::instance().render_im3d(
        primType,
        s_im3d_transformed.data(),
        static_cast<int>(s_im3d_transformed.size()),
        nullptr,
        0
    );
}

bool RwIm3DRenderIndexedPrimitive(RwPrimitiveType primType, const uint16_t* indices, int32_t numIndices) {
    if (!s_im3d_active || s_im3d_transformed.empty()) return false;
    return renderer::Renderer::instance().render_im3d(
        primType,
        s_im3d_transformed.data(),
        static_cast<int>(s_im3d_transformed.size()),
        indices,
        numIndices
    );
}

bool RwIm3DEnd() {
    s_im3d_active = false;
    s_im3d_transformed.clear();
    return true;
}

void RwCameraSetPerspective(float fov_degrees, float aspect, float near_clip, float far_clip) {
    renderer::Renderer::instance().set_perspective_projection(fov_degrees, aspect, near_clip, far_clip);
}

void RwCameraLookAt(float eye_x, float eye_y, float eye_z, float target_x, float target_y, float target_z) {
    renderer::Renderer::instance().set_view_matrix(eye_x, eye_y, eye_z, target_x, target_y, target_z);
}

// ----------------------------------------------------------------------------
// GTA San Andreas Loading / Splash Screen Subsystem
// ----------------------------------------------------------------------------

void CLoadingScreen_Init() {
    std::cout << "[LoadingScreen] Initializing splash screen subsystem...\n";
    CLoadingScreen_LoadSplashes();
    CLoadingScreen_RenderSplashWindow();
    std::cout << "[LoadingScreen] Initialized and displayed splash screen.\n";
}

void CLoadingScreen_Shutdown() {
    if (s_splashTexture) {
        RwTextureDestroy(s_splashTexture);
        s_splashTexture = nullptr;
        s_splashRaster = nullptr;
        std::cout << "[LoadingScreen] Splash textures destroyed.\n";
    }
}

void CLoadingScreen_RenderSplashWindow() {
    renderer::Renderer::instance().render_splash_screen();
}

RwRaster* CLoadingScreen_GetSplashRaster() {
    if (!s_splashRaster) {
        CLoadingScreen_LoadSplashes();
    }
    return s_splashRaster;
}

void CLoadingScreen_LoadSplashes() {
    if (s_splashTexture) return;

    // 1. Try loading authentic splash texture from game/models/txd/loadsc0.txd
    auto splash_opt = platform::VirtualFileSystem::instance().resolve("models/txd/loadsc0.txd");
    if (!splash_opt.has_value()) {
        splash_opt = platform::VirtualFileSystem::instance().resolve("models/txd/loadsc2.txd");
    }
    if (splash_opt.has_value()) {
        auto textures = assets::TxdLoader::load_from_file(*splash_opt);
        for (const auto& [name, tex] : textures) {
            if (tex && tex->raster) {
                s_splashTexture = tex;
                s_splashRaster = tex->raster;
                std::cout << "[LoadingScreen] Loaded authentic splash texture '" << name << "' ("
                          << tex->raster->width << "x" << tex->raster->height << ") from "
                          << splash_opt->filename().string() << ".\n";
                return;
            }
        }
    }

    // Fallback procedural gradient if game file not present
    constexpr int splash_w = 256;
    constexpr int splash_h = 256;
    s_splashRaster = RwRasterCreate(splash_w, splash_h, 32, 0);
    if (!s_splashRaster) return;

    uint8_t* pixels = RwRasterLock(s_splashRaster, 0, 0);
    if (pixels) {
        for (int y = 0; y < splash_h; ++y) {
            float t = static_cast<float>(y) / static_cast<float>(splash_h);
            uint8_t r = static_cast<uint8_t>((1.0f - t) * 230 + t * 40);
            uint8_t g = static_cast<uint8_t>((1.0f - t) * 120 + t * 20);
            uint8_t b = static_cast<uint8_t>((1.0f - t) * 40 + t * 100);
            uint32_t color = (0xFFu << 24) | (r << 16) | (g << 8) | b;

            auto* row = reinterpret_cast<uint32_t*>(pixels + y * s_splashRaster->stride);
            for (int x = 0; x < splash_w; ++x) {
                row[x] = color;
            }
        }
        RwRasterUnlock(s_splashRaster);
    }

    s_splashTexture = RwTextureCreate(s_splashRaster);
    std::strncpy(s_splashTexture->name, "loadscs", sizeof(s_splashTexture->name) - 1);
    std::cout << "[LoadingScreen] Fallback splash texture 'loadscs' created (256x256).\n";
}

// ----------------------------------------------------------------------------
// Texture & Raster Guest Address Mapping
// ----------------------------------------------------------------------------

uint32_t get_texture_guest_addr(RwTexture* tex) {
    if (!tex) return 0;
    auto it = s_texture_to_guest.find(tex);
    if (it != s_texture_to_guest.end()) {
        return it->second;
    }
    return tex->guest_addr;
}

RwTexture* get_texture_from_guest_addr(uint32_t addr) {
    if (!addr) return nullptr;
    auto it = s_guest_to_texture.find(addr);
    if (it != s_guest_to_texture.end()) {
        return it->second;
    }
    return nullptr;
}

uint32_t get_raster_guest_addr(RwRaster* raster) {
    if (!raster) return 0;
    auto it = s_raster_to_guest.find(raster);
    if (it != s_raster_to_guest.end()) {
        return it->second;
    }
    return raster->guest_addr;
}

RwRaster* get_raster_from_guest_addr(uint32_t addr) {
    if (!addr) return nullptr;
    auto it = s_guest_to_raster.find(addr);
    if (it != s_guest_to_raster.end()) {
        return it->second;
    }
    return nullptr;
}

// ----------------------------------------------------------------------------
// Authentic RenderWare Functions
// ----------------------------------------------------------------------------

RwTexture* RwTextureRead(const char* name, const char* mask) {
    (void)mask;
    if (!name || !name[0]) return nullptr;
    auto* tex = assets::TextureDictionary::instance().find_texture(name);
    if (tex) {
        ++tex->refCount;
        if (tex->guest_addr && runtime::GuestMemory::instance().is_initialized()) {
            runtime::GuestMemory::instance().write32(tex->guest_addr + 0x54, tex->refCount);
        }
    }
    return tex;
}

bool initialize_menu_and_font_textures() {
    static bool s_initialized = false;
    if (s_initialized) return true;

    auto& td = assets::TextureDictionary::instance();
    bool ok = true;
    ok &= td.load_dictionary("fonts", "models/fonts.txd");
    ok &= td.load_dictionary("frontend1", "models/fronten1.txd");
    ok &= td.load_dictionary("frontend2", "models/fronten2.txd");
    ok &= td.load_dictionary("frontend_pc", "models/fronten_pc.txd");

    // Initialize font sprites at 0x00C71AD0 (font2) and 0x00C71AD4 (font1)
    if (runtime::GuestMemory::instance().is_initialized()) {
        auto* f2 = td.find_texture("font2");
        auto* f1 = td.find_texture("font1");
        if (f2) {
            runtime::GuestMemory::instance().write32(0x00C71AD0, get_texture_guest_addr(f2));
        }
        if (f1) {
            runtime::GuestMemory::instance().write32(0x00C71AD4, get_texture_guest_addr(f1));
        }
        std::cout << "[RenderWare] Initialized CFont sprites (font1, font2) at 0x00C71AD0 / 0x00C71AD4.\n";
    }

    s_initialized = ok;
    return ok;
}

} // namespace pcrecomp::rw
