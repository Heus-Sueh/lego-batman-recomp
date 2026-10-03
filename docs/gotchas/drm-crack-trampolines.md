# Gotchas: DRM Protection and Crack Trampolines

> Review 2026-09-19: REVISION-SPECIFIC HISTORICAL LEADS. Section identity, address ranges and instruction examples below are UNKNOWN for a new binary until checked. Do not infer that all code in a range is disposable. Prefer compatible clean input; any necessary patch needs a verified preimage and preserved semantics.

## Background

Retail PC copies of *Grand Theft Auto: San Andreas* shipped with **SecuROM v7** copy protection. 

## 1. SecuROM v7 `.text` Encryption
- In encrypted retail binaries, the `.text` section on disk has an entropy greater than 7.9 (virtually random bytes).
- Linear disassembly of encrypted bytes produces garbage and illegal opcodes.
- Recompilation requires either:
  1. A clean, unpacked v1.0 US executable (`gta_sa_unpacked.exe`), or
  2. A memory dump captured after SecuROM unpacked the image in RAM.

## 2. The HOODLUM No-CD Crack Section
Many common community binaries (e.g., the Hoodlum release) bypassed the SecuROM disc check by overwriting early entry points with jumps into a dedicated section named `.HOODLUM` (`0x00401000..0x0040A000`).

### The Trampoline Pattern
Instead of direct jumps, the crack inserted small obfuscated math stubs to compute destinations dynamically:
```assembly
; Example crack trampoline at 0x0052F590:
mov eax, 0x0052F560
add eax, 0x00000030
inc eax
jmp eax ; computed destination
```

### Pitfalls to Avoid
1. **Do not lift the crack section**: The code inside `.HOODLUM` only checks for CD drivers, manipulates dummy files, and jumps back into genuine game code.
2. **De-obfuscate statically**: The destination can be computed statically at lift time using:
   $$\text{Destination} = \text{val1} + \text{val2} + 1$$
   By substituting the target jump with the genuine function entry address in `run_lift.py`, the lifted code executes authentic game logic without requiring the crack section or CD-check emulation.
