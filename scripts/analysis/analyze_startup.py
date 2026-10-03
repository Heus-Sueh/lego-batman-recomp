#!/usr/bin/env python3
"""
GTA: San Andreas Static Recompilation - Startup Frontier & ABI Validator
Milestone M2 Analysis Tool

Validates:
1. Functions fall within executable code regions in globals.toml.
2. Calling conventions and stack purge counts satisfy x86 ABI invariants.
3. Call graph reachability from CRT startup to game loop.
"""

import sys
import os
import tomllib
from pathlib import Path


def load_toml(path: Path) -> dict:
    if not path.exists():
        print(f"Error: {path} not found.", file=sys.stderr)
        sys.exit(1)
    with open(path, "rb") as f:
        return tomllib.load(f)


def validate_abi(fn: dict) -> list[str]:
    errors = []
    rva = fn.get("rva")
    name = fn.get("name", f"func_{hex(rva) if rva else 'unknown'}")
    conv = fn.get("convention", "")
    purge = fn.get("stack_purge", 0)

    if conv not in ("cdecl", "stdcall", "thiscall", "fastcall"):
        errors.append(f"Function {name} (0x{rva:08X}): unknown calling convention '{conv}'")

    if conv == "cdecl" and purge != 0:
        errors.append(f"Function {name} (0x{rva:08X}): cdecl must have stack_purge = 0 (got {purge})")

    if conv in ("stdcall", "thiscall", "fastcall"):
        if purge % 4 != 0:
            errors.append(f"Function {name} (0x{rva:08X}): stack_purge ({purge}) must be multiple of 4 on x86-32")

    return errors


def check_bounds(fn: dict, code_regions: list[dict]) -> bool:
    rva = fn.get("rva", 0)
    for reg in code_regions:
        if reg.get("permissions") in ("RX", "RWX") or reg.get("type") == "code":
            if reg["start"] <= rva <= reg["end"]:
                return True
    return False


def main():
    repo_root = Path(__file__).resolve().parent.parent.parent
    revision_dir = repo_root / "config" / "revisions" / "gta_sa_compact"

    functions_path = revision_dir / "functions.toml"
    globals_path = revision_dir / "globals.toml"

    print("=" * 70)
    print("  GTA: San Andreas Recomp - Startup Call Graph & ABI Analysis")
    print("=" * 70)

    funcs_data = load_toml(functions_path)
    globals_data = load_toml(globals_path)

    functions = funcs_data.get("function", [])
    regions = globals_data.get("regions", [])
    globals_list = globals_data.get("global", [])

    print(f"\n[Regions] Loaded {len(regions)} memory regions from globals.toml")
    for r in regions:
        print(f"  {r['name']:<12} [0x{r['start']:08X} - 0x{r['end']:08X}] ({r['permissions']}) {r['type']}")

    print(f"\n[Globals] Loaded {len(globals_list)} critical engine globals")
    for g in globals_list:
        print(f"  0x{g['address']:08X} {g['name']:<30} {g['type']}")

    print(f"\n[Functions] Loaded {len(functions)} discovered functions")

    # ABI & Bounds Validation
    abi_errors = []
    out_of_bounds = []

    fn_by_rva = {}
    for fn in functions:
        rva = fn["rva"]
        fn_by_rva[rva] = fn
        errs = validate_abi(fn)
        if errs:
            abi_errors.extend(errs)
        if not check_bounds(fn, regions):
            out_of_bounds.append(f"Function {fn['name']} (0x{rva:08X}) is outside known executable regions")

    if abi_errors:
        print("\n[ERROR] ABI Validation Failures:")
        for e in abi_errors:
            print(f"  - {e}")
    else:
        print("\n[PASS] All functions pass x86 ABI calling convention & stack purge invariants.")

    if out_of_bounds:
        print("\n[ERROR] Region Bounds Failures:")
        for e in out_of_bounds:
            print(f"  - {e}")
    else:
        print("[PASS] All functions reside within valid executable code sections.")

    # Reachability Traversal from CRT Entry (0x008246EF)
    entry_rva = 0x008246EF
    visited = set()
    queue = [entry_rva]

    while queue:
        curr = queue.pop(0)
        if curr in visited:
            continue
        visited.add(curr)
        fn = fn_by_rva.get(curr)
        if fn and "callees" in fn:
            for callee in fn["callees"]:
                if callee not in visited:
                    queue.append(callee)

    print(f"\n[Call Graph] Reachability from mainCRTStartup (0x{entry_rva:08X}):")
    reachable_funcs = [fn_by_rva[rva] for rva in visited if rva in fn_by_rva]
    for fn in sorted(reachable_funcs, key=lambda f: f["rva"]):
        print(f"  -> 0x{fn['rva']:08X} {fn['name']:<24} ({fn['convention']}) purge={fn['stack_purge']}")

    print(f"\nReachable slice: {len(reachable_funcs)} of {len(functions)} declared functions.")

    if abi_errors or out_of_bounds:
        sys.exit(1)

    print("\nMilestone M2 Static Analysis: SUCCESS")


if __name__ == "__main__":
    main()
