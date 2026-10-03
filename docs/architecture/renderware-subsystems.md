# RenderWare Subsystem Architecture in GTA: San Andreas

> Review 2026-09-19: CONCEPTUAL REFERENCE (INFERRED). Validate exact signatures, ABI and lifecycle ordering against the selected executable before using these descriptions as implementation instructions. Camera update completion and raster presentation must be checked separately.

## Overview

GTA: San Andreas (PC) is built on top of Criterion Games' **RenderWare Graphics Engine (v3.6/3.7)**. RenderWare is statically linked into `gta_sa.exe` rather than loaded as dynamic DLLs. It uses a driver abstraction and extensible plugin architecture.

## Engine Lifecycle & Core Subsystems

### 1. `RwEngine` Lifecycle
The engine initialization sequence in the binary proceeds as:
```text
RwEngineInit(RwDevice* device, RwUInt32 flags, RwUInt32 subSystemIndex)
    ↓
RwEngineOpen(RwEngineOpenParams* params)
    ↓
RwEngineStart()
    ↓
... game simulation & rendering loop ...
    ↓
RwEngineStop()
    ↓
RwEngineClose()
    ↓
RwEngineTerm()
```

- **`RwEngineInit`**: Discovers hardware subsystems, initializes memory allocators, and registers standard plugins.
- **`RwEngineOpen`**: Configures the display device, initializes windowing hooks, and binds rendering surfaces.
- **`RwEngineStart`**: Allocates driver resources, loads default rasters, and prepares the rendering pipeline.

### 2. `RwDevice` Hardware Interface
Hardware interaction is channeled through a virtual function table (`RwDeviceSystemFunction`). Key system requests dispatched through this interface include:
- `rwDEVICESYSTEMINIT`: Initializes the graphics subsystem (Direct3D 9 context creation).
- `rwDEVICESYSTEMOPEN`: Configures display mode and viewport parameters.
- `rwDEVICESYSTEMCLOSE`: Destroys the hardware device context.
- `rwDEVICESYSTEMGETNUMMODES`: Enumerates supported display modes and resolutions.
- `rwDEVICESYSTEMGETMODEINFO`: Retrieves resolution, color depth, and refresh rate for a given mode index.
- `rwDEVICESYSTEMSTANDARDS`: Queries device capabilities (multisampling, texture formats, shader support).

### 3. `RwCamera` Viewport Operations
The camera subsystem controls raster binding and frame presentation:
- `RwCameraBeginUpdate(RwCamera* camera)`: Binds target rasters (backbuffer and Z-buffer) to the hardware device and sets projection matrices.
- `RwCameraClear(RwCamera* camera, RwRGBA* color, RwInt32 clearFlags)`: Clears frame buffer and depth stencil.
- `RwCameraEndUpdate(RwCamera* camera)`: Finalizes frame rendering and triggers presentation (interception point for Tier 1 presentation pacing).

### 4. Scene Graph & Geometry (`RpWorld`, `RpClump`, `RpAtomic`)
- **`RpWorld`**: Container for spatial geometry, light sources, and sectors.
- **`RpClump`**: Hierarchical container for model instances (vehicles, pedestrians, dynamic props).
- **`RpAtomic`**: Renderable mesh instance containing geometry data, material tables, and pipeline pointers.

## Reverse-Engineering Anchors

To locate RenderWare subsystems in the disassembled binary:
1. **String Anchors**:
   - `"RwEngineInit"`, `"RwEngineStart"`, `"CameraBeginUpdate"`, `"Direct3D9"`
2. **Driver Table**:
   - Trace the device structure passed to `RwEngineInit`. Function pointers in this table represent the bridge between RenderWare and Direct3D 9.
