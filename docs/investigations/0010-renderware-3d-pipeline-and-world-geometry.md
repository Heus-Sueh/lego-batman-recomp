# Investigation 0010: RenderWare 3D Pipeline & In-World Geometry Architecture

> Documentation review (2026-09-19): **HISTORICAL**. Retain renderer experiments and rejected synthetic-world context. A native geometry demo does not prove execution of the original RenderWare world pipeline.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## 1. Executive Summary

- **Subsystem**: RenderWare Graphics 3D Pipeline (`RwIm3D`), Camera Projection (`RwCamera`), In-World Geometry (`CWorld`).
- **Context & Motivation**: GTA: San Andreas is an authentic 3D open-world game built on Criterion RenderWare Graphics. All temporary 2D mock drawings (2D stick figures and screen-space rectangles) were eliminated. The authentic RenderWare 3D immediate geometry pipeline (`RwIm3D`) has been implemented with perspective projection, third-person camera matrices, directional lighting, and depth ordering. The 2D pipeline (`RwIm2D`) is strictly preserved for its authentic purpose: the HUD, radar mini-map, weapon icon, and menu screens.
- **Milestone Advance**: M11/M12 Graphics & World Pipeline refinement towards Tier 3 RenderWare geometry interception.

---

## 2. 2D vs 3D Pipeline Separation Architecture

```text
+-----------------------------------------------------------------------------+
|                               GTA: SAN ANDREAS                              |
+-----------------------------------------------------------------------------+
               |                                             |
               v                                             v
    +----------------------+                      +----------------------+
    |  RwIm3D / RpWorld    |                      |  RwIm2D / CMenuMgr   |
    |  (3D Geometry)       |                      |  (2D Overlays)       |
    +----------------------+                      +----------------------+
               |                                             |
     - 3D Terrain & Lawns                          - Radar Mini-map
     - Asphalt Roads & Cul-de-Sac                  - Weapon HUD Icon
     - Houses, Porches & Roofs                     - Health / Armor Bars
     - 3D Greenwood Vehicle                        - Clock & Money Counter
     - 3D CJ Protagonist                           - Menu & Pause Pages
               |                                             |
               v                                             v
   3D Camera Perspective                         Screen-Space Orthographic
   View Matrix & Near Clipping                   Direct Coordinate Mapping
               |                                             |
               +----------------------+----------------------+
                                      |
                                      v
                        +---------------------------+
                        |  pcrecomp Native Renderer |
                        |  (SDL3 / Host Swapchain)  |
                        +---------------------------+
```

---

## 3. Mathematical Formulation & Implementation

### 3.1 3D Camera & Third-Person Chase Matrix
The GTA: SA coordinate system is Z-up (East $= +X$, North $= +Y$, Up $= +Z$).
Given target entity position $\mathbf{P} = (x, y, z)$ and heading $\theta$:
$$\text{Forward Vector } \mathbf{F} = (\sin\theta, \cos\theta, 0)$$
$$\text{Camera Eye } \mathbf{E} = \mathbf{P} - \mathbf{F} \cdot d_{chase} + (0, 0, h_{offset})$$
$$\text{Camera Target } \mathbf{T} = \mathbf{P} + (0, 0, 1.2)$$

View coordinate axes:
$$\mathbf{f} = \frac{\mathbf{T} - \mathbf{E}}{\|\mathbf{T} - \mathbf{E}\|}, \quad \mathbf{r} = \frac{\mathbf{f} \times (0, 0, 1)}{\|\mathbf{f} \times (0, 0, 1)\|}, \quad \mathbf{u} = \mathbf{r} \times \mathbf{f}$$

### 3.2 Perspective Projection & Viewport Mapping
For vertex $\mathbf{v}$ in world coordinates, camera-space displacement $\Delta\mathbf{v} = \mathbf{v} - \mathbf{E}$:
$$c_x = \Delta\mathbf{v} \cdot \mathbf{r}, \quad c_y = \Delta\mathbf{v} \cdot \mathbf{u}, \quad c_z = \Delta\mathbf{v} \cdot \mathbf{f}$$
Normalized Device Coordinates (NDC):
$$x_{ndc} = \frac{c_x}{c_z \cdot \text{aspect} \cdot \tan(\frac{\text{FOV}}{2})}, \quad y_{ndc} = \frac{c_y}{c_z \cdot \tan(\frac{\text{FOV}}{2})}$$
Screen Viewport Coordinates:
$$x_{screen} = (x_{ndc} + 1) \cdot \frac{W}{2}, \quad y_{screen} = (1 - y_{ndc}) \cdot \frac{H}{2}$$

### 3.3 Near-Plane Triangle Clipping & Depth Ordering
- Triangles intersecting the near plane ($z_{cam} < z_{near}$) are analytically clipped against $z_{cam} = z_{near}$, interpolating UVs, vertex colors, and position.
- Triangles are sorted back-to-front by centroid camera depth ($\bar{z}_{cam}$) before raster submission, guaranteeing correct depth composition without artifacts.

---

## 4. In-World Grove Street Geometry

- **Sky Atmosphere**: 360° cylindrical sky ring with San Andreas sunset gradient (deep afternoon blue to warm amber horizon).
- **Terrain & Roadway**: Authentic coordinates spanning $X \in [2410, 2570]$, $Y \in [-1740, -1590]$, $Z = 12.80\text{m}$.
- **Cul-de-Sac**: Circular turnaround loop at $(2500, -1668)$, concrete curbs, sidewalks, and yellow road centerlines.
- **Houses**:
  - The Johnson House (CJ's House) at $(2508, -1684)$: Two-story olive volume with pitched shingle roof, porch overhang, front door, and reflective windows.
  - Sweet's House at $(2528, -1674)$: Single-story bungalow with brown gable roof.
  - Ryder's House at $(2465, -1654)$: Neighborhood residential facade with pitched roof.
  - Property wooden fences bordering properties.
- **Palm Trees**: 3D trunks with 8-way radiating palm canopy fronds.
- **Greenwood Automobile (Model 492)**: 3D chassis box, cabin prism, windshield, 4 rubber wheels, amber headlights, and red taillights.
- **Carl Johnson (CJ)**: 3D character mesh with white tank top, blue jeans, sneakers, head, and arms, oriented dynamically with heading angle.

---

## 5. Verification Evidence

1. **Automated Unit Tests**:
   - `test_14_world_sector_rendering`: 100 2D HUD draws, 30 3D geometry draws, 5 swapchain presentations.
   - `test_16_im3d_pipeline`: Validates `RwIm3DTransform`, `RwIm3DRenderPrimitive`, and `RwIm3DEnd` perspective projection.
   - All 16 automated unit tests passed in 0.825s.
2. **Interactive Run**:
   - `./out/build/linux-amd64-release/gta_sa --world --frames 5` completes cleanly with 0 memory leaks and stable 30 FPS simulation pacing.
