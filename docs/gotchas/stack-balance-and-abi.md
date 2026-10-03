# Gotchas: Stack Balance and ABI Conventions

## The Problem

In an x86-32 static recompilation, stack balance is strictly unforgiving. A single stack purge error or unpreserved register causes memory corruption that usually does not crash at the call site, but rather dozens of calls later when an innocent caller pops an invalid return address or base pointer.

## 1. Callee vs Caller Cleanup

| Convention | Stack Cleanup | Epilogue Instruction | Call Site Sequence |
|---|---|---|---|
| `cdecl` | **Caller** cleans stack | `ret` (or `ret 0`) | `push arg1; call fn; add esp, 4` |
| `stdcall` | **Callee** cleans stack | `ret N` (bytes of stack arguments) | `push arg1; call fn` (no `add esp`) |
| `thiscall` | **Callee** cleans stack | `ret N` (bytes of stack arguments) | `mov ecx, this; push arg1; call fn` |
| `fastcall` | **Callee** cleans stack | `ret N` | `mov ecx, a1; mov edx, a2; call fn` |

## 2. Common Decompiler Illusions

> [!WARNING]
> **Decompiler signatures cannot be trusted without binary confirmation.**

- **Decompiler shows `cdecl`**: Ghidra or IDA often defaults to `cdecl` for functions whose prototypes are unclassified. However, if the function ends with `ret 0x10`, the instruction establishes callee cleanup of 16 bytes. It does not distinguish `stdcall`, `thiscall`, `fastcall` or a custom ABI by itself. Forcing `cdecl` will leave 16 bytes of arguments on the stack, corrupting subsequent calls.
- **Decompiler omits `thiscall`**: If the decompiler fails to detect `ecx` being used as the `this` pointer, it may treat the first stack argument as `this` or mistake member accesses (`[ecx + 0x14]`) for global accesses.
- **Variable Argument Functions (`printf`, `sprintf`)**: Must always be `cdecl`. The callee cannot know how many arguments were passed at runtime, so it cannot purge the stack.

## 3. The Recomp ICALL Bridge Trap

When lifting an indirect call (`RECOMP_ICALL`), the dispatcher pushes a synthetic 4-byte return address to simulate a hardware `call` instruction:

```text
[esp + 0]: synthetic return address (4 bytes)
[esp + 4]: first function argument
[esp + 8]: second function argument
```

When implementing a manual bridge or C++ shim:
- **`cdecl` function**: Callee MUST pop the return address (`esp += 4`). The caller will clean the remaining arguments (`add esp, N`).
- **`stdcall` / `thiscall` function**: Callee MUST pop the return address AND all stack arguments (`esp += 4 + stack_purge_bytes`).

Failure to pop the synthetic return address causes the caller to return into the arguments!
