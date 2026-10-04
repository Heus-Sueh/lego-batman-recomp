# External Reference: pcrecomp Upstream Baseline

> Review 2026-09-19: OWNERSHIP UPDATE. Generic runtime foundations now live in `libpcrecomp/`, `win32compat/`, and `pcmodern/`; codegen remains in the toolbox and title metadata/integration in the project. See [runtime architecture](../runtime-architecture.md).

## Overview

For the selective heap-boundary adaptation from the newer upstream runtime,
see [win32hle integration](win32hle-integration.md).

The `pcrecomp` toolbox (https://github.com/sp00nznet/pcrecomp) provides the core analysis, disassembly, and instruction-lifting pipeline for this project. It is integrated as a git submodule in `thirdparty/pcrecomp/`.

## Upstream Pipeline Components

```text
tools/
├── pe/
│   ├── pe_analyze.py        # Section analysis, image base, entry point, relocations
│   ├── analyze_sections.py   # Section entropy and protection detection
│   └── extract_imports.py   # IAT parsing, ordinal resolution, stack purge calculation
├── disasm/
│   └── disasm32.py          # Recursive descent disassembly, basic blocks, jump tables
└── lift/
    └── lift32.py            # x86-32 instruction lifting to C/C++ AST
```

## Boundary Rules & Upstream Candidates

To maintain clean separation between the generic recompiler toolbox and this title-specific project:

### Belongs Upstream (`thirdparty/pcrecomp/`)
Fixes and improvements should be contributed upstream when they resolve:
- Generic PE header or section table parsing bugs.
- x86-32 instruction lifting, opcode decoding, or flag semantics.
- General control flow graph (CFG) recovery or jump table patterns.
- Generic calling convention analysis (`cdecl`, `stdcall`, `thiscall`, `fastcall`).
- Generic C/C++ codegen quality or compiler compatibility (Clang, GCC, MSVC).

### Belongs in Project (`src/`, `config/`, `docs/`)
Keep game-specific logic and data in this repository:
- Revision metadata (`config/revisions/gta_sa_<sha>/`).
- Function maps and symbol names (`functions.toml`).
- Game-specific deterministic patches (`patches.toml`).
- Win32 / DirectInput / DirectSound compatibility shims (`src/win32/`).
- RenderWare engine integration and presentation pacing (`src/renderer/`, `src/platform/`).

## Upstream Isolation Invariant

- Upstream modifications should remain isolated on dedicated branches or local commits within `thirdparty/pcrecomp` so they can be submitted cleanly as pull requests.
- Never place title-specific hacks or GTA SA addresses directly inside `thirdparty/pcrecomp/`.
