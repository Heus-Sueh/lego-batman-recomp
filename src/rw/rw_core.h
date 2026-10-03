#pragma once

#include <cstdint>
#include "../runtime/guest_types.h"

namespace pcrecomp::rw {

// RenderWare standard structures
struct RwRGBA {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};
    uint8_t a{255};
};

struct RwCamera {
    void*   frame{nullptr};
    void*   buffer{nullptr};
    int32_t width{800};
    int32_t height{600};
};

struct RwRaster {
    struct RwRaster* parent{nullptr};
    uint8_t*         cpPixels{nullptr};
    uint8_t*         palette{nullptr};
    int32_t          width{0};
    int32_t          height{0};
    int32_t          depth{32};
    int32_t          stride{0};
    int16_t          nOffsetX{0};
    int16_t          nOffsetY{0};
    uint8_t          cType{0};
    uint8_t          cFlags{0};
    uint8_t          privateFlags{0};
    uint8_t          cFormat{0};
    uint8_t*         originalPixels{nullptr};
    int32_t          originalWidth{0};
    int32_t          originalHeight{0};
    int32_t          originalStride{0};
    uint32_t         guest_addr{0};
};

struct RwTexture {
    RwRaster* raster{nullptr};
    void*     dict{nullptr};
    void*     next{nullptr};
    void*     prev{nullptr};
    char      name[32]{0};
    char      mask[32]{0};
    uint32_t  filterAddressing{0};
    int32_t   refCount{1};
    uint32_t  guest_addr{0};
};

// 3D Math Structures
struct RwV3d {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

struct RwMatrix {
    RwV3d    right{1.0f, 0.0f, 0.0f};
    uint32_t flags{0};
    RwV3d    up{0.0f, 1.0f, 0.0f};
    uint32_t pad1{0};
    RwV3d    at{0.0f, 0.0f, 1.0f};
    uint32_t pad2{0};
    RwV3d    pos{0.0f, 0.0f, 0.0f};
    uint32_t pad3{0};
};

// 2D Immediate-Mode Primitives
struct RwIm2DVertex {
    float    x{0.0f};
    float    y{0.0f};
    float    z{0.0f};
    float    rhw{1.0f};
    uint32_t emissiveColor{0xFFFFFFFF}; // ARGB
    float    u{0.0f};
    float    v{0.0f};
};

// 3D Immediate-Mode Primitives
struct RwIm3DVertex {
    RwV3d    objVertex{0.0f, 0.0f, 0.0f}; // 3D Object Space Coordinate
    RwV3d    objNormal{0.0f, 1.0f, 0.0f}; // 3D Normal
    uint32_t color{0xFFFFFFFF};           // Diffuse color (ARGB)
    float    u{0.0f};
    float    v{0.0f};
};

enum RwPrimitiveType : uint32_t {
    rwPRIMTYPENAL       = 0,
    rwPRIMTYPELINELIST  = 1,
    rwPRIMTYPEPOLYLINE  = 2,
    rwPRIMTYPETRILIST   = 3,
    rwPRIMTYPETRISTRIP  = 4,
    rwPRIMTYPETRIFAN    = 5,
    rwPRIMTYPEPOINTLIST = 6
};

// RenderWare application event IDs
enum RsEvent : uint32_t {
    rsINIT                 = 0,
    rsREGISTERIMAGEFORMATS = 1,
    rsRWINIT               = 2,
    rsRWTERMINATE          = 3,
    rsIDLE                 = 4,
    rsFRONTENDQUITREQUEST  = 5,
    rsACTIVATE             = 6,
    rsKEYDOWN              = 7,
    rsKEYUP                = 8,
    rsMOUSEMOVE            = 9,
    rsMOUSEBUTTONDOWN      = 10,
    rsMOUSEBUTTONUP        = 11,
    rsPADBUTTONDOWN        = 12,
    rsPADBUTTONUP          = 13,
    rsPADANALOGUEMOVE      = 14
};

// Subsystem Lifecycle
bool initialize_renderware_subsystems();
void shutdown_renderware_subsystems();

// Camera Lifecycle Hooks
void* RwCameraBeginUpdate(void* camera);
void* RwCameraEndUpdate(void* camera);
void* RwCameraClear(void* camera, const RwRGBA* colour, uint32_t clearFlags);

// RenderWare Application Event Dispatcher
bool dispatch_rs_event(uint32_t event, void* param);

// Tier 2 Resource Interception: Rasters & Textures
RwRaster* RwRasterCreate(int32_t width, int32_t height, int32_t depth, int32_t flags);
bool      RwRasterDestroy(RwRaster* raster);
uint8_t*  RwRasterLock(RwRaster* raster, uint8_t level, int32_t lockMode);
RwRaster* RwRasterUnlock(RwRaster* raster);
RwRaster* RwRasterShowRaster(RwRaster* raster, void* dev, uint32_t flags);

RwTexture* RwTextureCreate(RwRaster* raster);
bool       RwTextureDestroy(RwTexture* texture);

// 2D Immediate Mode Geometry Submission
bool RwIm2DRenderPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, int32_t numVertices);
bool RwIm2DRenderIndexedPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, int32_t numVertices, uint16_t* indices, int32_t numIndices);

// 3D Immediate Mode Geometry Submission (RwIm3D)
void* RwIm3DTransform(RwIm3DVertex* pVerts, uint32_t numVerts, const RwMatrix* ltm = nullptr, uint32_t flags = 0);
bool  RwIm3DRenderPrimitive(RwPrimitiveType primType);
bool  RwIm3DRenderIndexedPrimitive(RwPrimitiveType primType, const uint16_t* indices, int32_t numIndices);
bool  RwIm3DEnd();

// 3D Camera Configuration
void RwCameraSetPerspective(float fov_degrees, float aspect, float near_clip, float far_clip);
void RwCameraLookAt(float eye_x, float eye_y, float eye_z, float target_x, float target_y, float target_z);

// GTA San Andreas Loading / Splash Screen Subsystem
void CLoadingScreen_Init();
void CLoadingScreen_Shutdown();
void CLoadingScreen_RenderSplashWindow();
void CLoadingScreen_LoadSplashes();
RwRaster* CLoadingScreen_GetSplashRaster();

// Texture & Raster Guest Address Mapping
uint32_t   get_texture_guest_addr(RwTexture* tex);
RwTexture* get_texture_from_guest_addr(uint32_t addr);
uint32_t   get_raster_guest_addr(RwRaster* raster);
RwRaster*  get_raster_from_guest_addr(uint32_t addr);

// Authentic RenderWare Functions
RwTexture* RwTextureRead(const char* name, const char* mask = nullptr);
bool       initialize_menu_and_font_textures();

} // namespace pcrecomp::rw
