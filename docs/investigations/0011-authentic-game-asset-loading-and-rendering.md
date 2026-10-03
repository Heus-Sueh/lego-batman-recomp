# Investigation 0011: Authentic Game Asset Loading and In-Game Rendering

> Documentation review (2026-09-19): **HISTORICAL**. Retain asset-format and parser discoveries. Existing parsers and a loaded splash do not establish a complete streaming pipeline, menu correctness or gameplay.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Executive Summary

Following the user directive to eliminate mockups and load authentic game assets for menus and gameplay, we implemented a complete native RenderWare binary asset parsing and streaming pipeline:
1. **RenderWare TXD Texture Dictionaries** (`src/assets/txd_loader`): Direct decoding of DXT1, DXT3, DXT5 compressed and 8/32-bit paletted textures into native `RwRaster` and `RwTexture` instances.
2. **GTA Archive Format v2** (`src/assets/img_archive`): 2048-byte sector-aligned index and file extraction from `models/gta3.img` (16,316 assets) and `models/player.img` (542 assets).
3. **RenderWare DFF Clump/Geometry 3D Meshes** (`src/assets/dff_loader`): Chunk traversal (`rwID_CLUMP` `0x10`, `rwID_GEOMETRYLIST` `0x1A`, `rwID_GEOMETRY` `0x0F`, `rwID_BINMESHPLUGIN` `0x050E`), extracting vertices, surface normals, texture UVs, and triangle strips/lists.
4. **GTA Text Dictionaries** (`src/assets/gxt_loader`): Binary `american.gxt` parser loading 16,588 official localized text entries.
5. **In-World & Frontend Integration**:
   - **Frontend**: Authentic menu background artwork `back2` from `models/fronten2.txd` and localized strings from `american.gxt`.
   - **Loading Splash**: Authentic loading splash screen `loadsc0` (512x512) from `models/txd/loadsc0.txd`.
   - **Grove Street World Geometry**: Authentic cul-de-sac roadway (`lae2_roads89.dff`), Carl Johnson's house (`carlshou1_lae2.dff`), Sweet's house (`sweetshou1_lae2.dff`), and Ryder's house (`rydhou01_lae2.dff`) extracted from `models/gta3.img`.
   - **Automobile**: Authentic Greenwood sedan (`greenwoo.dff`, 21 geometries, 4,034 vertices) dynamically oriented with vehicle physics.
   - **Protagonist (CJ)**: Authentic composite character model from `models/player.img` (`head.dff`, `vest.dff`, `jeans.dff`, `sneaker.dff`) perfectly aligned to the skeletal origin.
   - **HUD**: Authentic radar mini-map disc (`radardisc`), unarmed fist icon (`fist`), and player blip (`radar_CJ`) loaded from `models/hud.txd`.

---

## Architectural Details

### 1. Asset Parsers (`src/assets/`)

| Parser | Format | Source | Key Capabilities |
|---|---|---|---|
| `ImgArchive` | GTA Archive v2 (`VER2`) | `models/gta3.img`, `models/player.img` | Binary directory table index, sector-offset streaming, fast file lookup. |
| `TxdLoader` | RenderWare TXD 3.6 (`0x16`) | `models/hud.txd`, `models/fronten2.txd`, `models/txd/loadsc0.txd` | DXT1, DXT3, DXT5 block decompression, 8-bit palette expansion, `RwRaster` texture binding. |
| `DffLoader` | RenderWare DFF 3.6 (`0x10`) | `gta3.img`, `player.img` | Clump hierarchy traversal, `BinMeshPlugin` tristrip conversion, 3D world space transformation. |
| `GxtLoader` | GTA GXT Text | `text/american.gxt` | TKEY / TDAT table binary lookup, 16,588 key/value string pairs. |

### 2. Authentic Model Placements

Coordinates matching `data/maps/LA/LAe2.ipl`:
- **Carl's House (`carlshou1_lae2`)**: `(2494.27, -1696.21, 17.05)`, $\text{Yaw} = 180^\circ$.
- **Sweet's House (`sweetshou1_lae2`)**: `(2529.89, -1677.66, 16.73)`, $\text{Yaw} = 0^\circ$.
- **Ryder's House (`rydhou01_lae2`)**: `(2457.84, -1695.94, 14.29)`, $\text{Yaw} = 90^\circ$.
- **Cul-de-sac Roadway (`lae2_roads89`)**: `(2489.30, -1668.50, 12.30)`, $\text{Yaw} = 0^\circ$.
- **Greenwood Sedan (`greenwoo`)**: Dynamic vehicle coordinates at `(2505.0, -1672.0, 13.95)`.
- **CJ Character (`head`, `vest`, `jeans`, `sneaker`)**: Dynamic player coordinates at `(2490.0, -1668.0, 14.34)`.

---

## Validation Evidence

### 1. Build Verification
```bash
$ just build
```
Clean build with GCC 14 and Ninja.

### 2. In-Game Asset Loading & Rendering
```bash
$ ./out/build/linux-amd64-release/gta_sa --world --frames 5
```
Output:
```text
[World] Loaded authentic HUD textures (radardisc, fist, radar_CJ) from hud.txd.
[ImgArchive] Opened gta3.img (16316 entries loaded)
[World] Opened models/gta3.img (16316 entries).
[DffLoader] Loaded model 'greenwoo' (21 geometries)
[DffLoader] Loaded model 'carlshou1_lae2' (1 geometries)
[DffLoader] Loaded model 'sweetshou1_lae2' (1 geometries)
[DffLoader] Loaded model 'rydhou01_lae2' (1 geometries)
[DffLoader] Loaded model 'lae2_roads89' (1 geometries)
[ImgArchive] Opened player.img (542 entries loaded)
[World] Opened models/player.img (542 entries).
[DffLoader] Loaded model 'head' (1 geometries)
[DffLoader] Loaded model 'vest' (1 geometries)
[DffLoader] Loaded model 'jeans' (1 geometries)
[DffLoader] Loaded model 'sneaker' (1 geometries)
[World] Initialized CWorld gameplay sector: Grove Street, Los Santos (3D RenderWare world active).
...
[Renderer] Frame presentation stats:
  Total frame passes:     10
  Swapchain presentations: 5
  Simulation ticks:        1
  2D primitive draws:      85
  3D primitive draws:      170
  Average presentation FPS:106.383
```

### 3. Full Test Suite
```bash
$ just test
```
```text
Ran 17 tests in 1.424s

OK
```
All 17 tests passed cleanly.
