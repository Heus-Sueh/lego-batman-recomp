# Investigation 0001: Executable Identification & SecuROM Analysis

> Documentation review (2026-09-19): **HISTORICAL**. Retain binary provenance and section evidence. The retail fingerprint and old entrypoint labels below do not identify the currently loaded image without a fresh hash.
> Evidence status: **UNKNOWN for present-day claims** unless independently revalidated.
> See the [documentation review](../documentation-review.md) and [current runtime architecture](../runtime-architecture.md).

## Overview

- **Executable**: `game/GTA San Andreas/gta_sa.exe` (symlinked as `game/gta_sa.exe`)
- **Investigation Date**: 2026-09-13
- **Phase Gate**: Advances Milestone M1 (Binary Inventory) and prepares Milestone M2 (Analysis)

---

## 1. Binary Identification & Hashes

| Property | Value |
|---|---|
| **File Name** | `gta_sa.exe` |
| **File Size** | 14,405,632 bytes |
| **SHA-256** | `8c609f108ad737deffbd0d17c702f5974d290c4379de742277b809f80350da1c` |
| **SHA-1** | `0df50d56ef9e304c8d59366afa9aa5c71159261d` |
| **PE Machine** | `0x014C` (i386 32-bit) |
| **PE Subsystem** | Windows GUI |
| **Image Base** | `0x00400000` |
| **PE Timestamp** | `0x4270F18A` (2005-04-28 14:22:02 UTC) |
| **Linker Version** | 7.00 (MSVC 2002/2003) |
| **Entry Point RVA** | `0x00EFFE40` (VA `0x012FFE40`) |

This fingerprint uniquely identifies the **original European/American Retail DVD-ROM v1.0 release** of Grand Theft Auto: San Andreas.

---

## 2. Section Layout & Entropy Analysis

Analysis executed via `thirdparty/pcrecomp/tools/pe/analyze_sections.py`:

```text
name               VA     vsize   rawsize  entropy  flags
------------------------------------------------------------------------------
.text      0x00001000   4545754   4546560   7.9944  EXEC HIGH-ENTROPY
_rwcseg    0x00457000      1105      4096   2.9420  EXEC
.rdata     0x00458000    307868    311296   6.0415  
.data      0x004A4000   4166708    262144   5.3192  
_TEXT_HA   0x0089E000     68226     69632   6.6135  
_rwdseg    0x008AF000         8      4096   0.0000  
.rsrc      0x008B0000      1312      4096   1.5416  
.text      0x008B1000   6593312   6594560   7.9938  EXEC HIGH-ENTROPY
.init      0x00EFB000     22704     24576   6.1012  EXEC ENTRY-POINT
.data      0x00F01000   2442180   2445312   0.0173  EXEC
.securom   0x01156000    135168    135168   7.5719  EXEC HIGH-ENTROPY
```

### Key Findings
1. **DRM Protection**: **SecuROM v7**.
   - Entry point is located inside section 9 (`.init`) at RVA `0x00EFFE40`, which is the SecuROM runtime decrypter stub.
   - SecuROM signatures matched: `AddD` at offset `0xCDB0F0`, `SecuROM` string at `0xC6C0D1`.
2. **Encrypted Game Code**:
   - The game's original `.text` section (RVA `0x00001000` to `0x00456CDA`, containing game functions from `0x00401000` to `0x00856CDA`) has an entropy of **7.9944 / 8.0000**.
   - The bytes on disk are ciphertext encrypted by SecuROM. Attempting raw linear or recursive-descent disassembly on this disk section yields invalid opcodes.

---

## 3. Recompilation Strategy ("Support Both")

To maintain absolute fidelity to the behavioral reference while enabling practical static code lifting:

1. **Manifest for Retail Revision (`config/revisions/gta_sa_8c609f10/`)**:
   - Records the official retail DVD-ROM executable fingerprint, section table, and imports.
   - Preserves provenance for retail media owners.
2. **Manifest for Clean Unpacked Revision (`config/revisions/gta_sa_compact/`)**:
   - Targets the decrypted 32-bit x86 image where `.text` contains clean instructions.
   - Entry point: `mainCRTStartup` at `0x008246EF`.
   - Application entry: `WinMain` at `0x00748DA0`.
   - Core init anchors: `RwEngineInit` (`0x007F6F10`), `CGame_Initialise` (`0x0053BC20`).
   - Enables `disasm32.py` and `lift32.py` to generate clean, readable C++ without DRM artifacts.

---

## 4. Verification

- `config/revisions/gta_sa_8c609f10/pe_analysis.json` generated and validated.
- `config/revisions/gta_sa_8c609f10/imports_raw.txt` and `imports.toml` cataloged 265 APIs across 10 DLLs.
- `config/revisions/gta_sa_8c609f10/revision.toml` created with exact section definitions.
