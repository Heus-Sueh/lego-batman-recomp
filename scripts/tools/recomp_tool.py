#!/usr/bin/env python3
"""
Static recompilation developer automation CLI for the configured target.
High-efficiency tool to inspect functions, trace unlifted calls, auto-add to functions.toml,
and run rapid development/validation cycles.
"""

import sys
import os
import re
import json
import subprocess
import hashlib
import tomllib
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
with open(REPO_ROOT / "config" / "target.toml", "rb") as f:
    TARGET = tomllib.load(f)["target"]
CONFIG_DIR = REPO_ROOT / TARGET["active_revision"]
FUNCTIONS_TOML = CONFIG_DIR / "functions.toml"
FUNCTIONS_JSON = CONFIG_DIR / "functions.json"
CACHE_FILE = REPO_ROOT / "generated" / "recomp" / ".lift_cache"
EXE_CANDIDATES = [REPO_ROOT / TARGET["default_game_dir"] / TARGET["guest_executable"]]

def find_game_exe() -> Path:
    for candidate in EXE_CANDIDATES:
        if candidate.exists():
            return candidate
    print("[Error] No game executable found in game/ directory.", file=sys.stderr)
    sys.exit(1)

def get_pe_and_capstone():
    try:
        import pefile
        import capstone
        return pefile, capstone
    except ImportError as e:
        print(f"[Error] Required Python package missing: {e}. Run via .venv/bin/python3.", file=sys.stderr)
        sys.exit(1)

def parse_va(va_str: str) -> int:
    va_str = va_str.strip()
    if va_str.lower().startswith("0x"):
        return int(va_str, 16)
    return int(va_str, 10 if va_str.isdigit() else 16)

_cached_pe = None
_cached_md = None

def analyze_function_binary(exe_path: Path, va: int):
    global _cached_pe, _cached_md
    pefile, capstone = get_pe_and_capstone()
    if _cached_pe is None:
        _cached_pe = pefile.PE(str(exe_path), fast_load=True)
    if _cached_md is None:
        _cached_md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        _cached_md.detail = True
    pe = _cached_pe
    md = _cached_md
    image_base = pe.OPTIONAL_HEADER.ImageBase
    rva = va - image_base

    try:
        data = pe.get_data(rva, 8192)
    except Exception as e:
        return None

    insns = []
    leaders = {va}
    max_target = va
    ret_found = False
    stack_purge = 0
    end_addr = va
    reads_ecx_first = False
    writes_ecx_first = False

    for ins in md.disasm(data, va):
        insns.append(ins)
        curr = ins.address

        # Register usage check for thiscall
        if not reads_ecx_first and not writes_ecx_first:
            for op in ins.operands:
                if op.type == capstone.x86.X86_OP_MEM:
                    if op.value.mem.base == capstone.x86.X86_REG_ECX:
                        reads_ecx_first = True
                elif op.type == capstone.x86.X86_OP_REG:
                    if op.value.reg == capstone.x86.X86_REG_ECX:
                        if ins.mnemonic in ("mov", "xor", "sub", "lea", "pop") and ins.operands[0].value.reg == capstone.x86.X86_REG_ECX:
                            writes_ecx_first = True
                        else:
                            reads_ecx_first = True

        # Branch targets (jumps only, calls are inter-procedural)
        if ins.group(capstone.CS_GRP_JUMP):
            if ins.operands and ins.operands[0].type == capstone.x86.X86_OP_IMM:
                tgt = ins.operands[0].value.imm
                if va <= tgt < va + 65536:
                    leaders.add(tgt)
                    if tgt > max_target:
                        max_target = tgt

        # Return instruction
        if ins.mnemonic == "ret":
            ret_found = True
            if ins.operands and ins.operands[0].type == capstone.x86.X86_OP_IMM:
                stack_purge = ins.operands[0].value.imm
            if curr >= max_target:
                end_addr = curr + ins.size
                break
        elif ins.mnemonic == "int3":
            if curr >= max_target and ret_found:
                end_addr = curr
                break

    if not insns:
        return None

    # Scan for alignment padding after end_addr
    offset = end_addr - va
    padding_len = 0
    while offset + padding_len < len(data):
        b = data[offset + padding_len]
        if b in (0x90, 0xCC): # NOP or INT3
            padding_len += 1
        else:
            break
    total_size = (end_addr - va) + padding_len

    # Convention inference
    if stack_purge > 0:
        convention = "thiscall" if reads_ecx_first else "stdcall"
    else:
        convention = "thiscall" if reads_ecx_first else "cdecl"

    return {
        "va": va,
        "size": total_size,
        "code_size": end_addr - va,
        "stack_purge": stack_purge,
        "convention": convention,
        "num_instructions": len(insns),
        "instructions": insns,
        "end_va": va + total_size
    }

def cmd_inspect(args):
    if not args:
        print("Usage: recomp_tool.py inspect <address>")
        return
    va = parse_va(args[0])
    exe_path = find_game_exe()
    info = analyze_function_binary(exe_path, va)
    if not info:
        print(f"Failed to analyze function at 0x{va:08X}")
        return

    print("=" * 70)
    print(f"  Function Inspection: 0x{va:08X}")
    print("=" * 70)
    print(f"  Size:             {info['size']} bytes (Code: {info['code_size']} bytes)")
    print(f"  Instructions:     {info['num_instructions']}")
    print(f"  Stack Purge:      {info['stack_purge']} bytes")
    print(f"  Inferred Conv:    {info['convention']}")
    print(f"  End VA:           0x{info['end_va']:08X}")
    print("-" * 70)
    for ins in info["instructions"]:
        bytes_hex = " ".join(f"{b:02x}" for b in ins.bytes)
        print(f"  0x{ins.address:08X}  {bytes_hex:<16}  {ins.mnemonic:<7} {ins.op_str}")
    print("=" * 70)

def find_recomp_bin() -> Path:
    candidates = [
        REPO_ROOT / "out" / "build" / "linux-amd64-release" / TARGET["output_name"],
        REPO_ROOT / "out" / "build" / "linux-amd64-debug" / TARGET["output_name"],
        REPO_ROOT / "out" / "build" / "win-amd64-release" / f"{TARGET['output_name']}.exe",
    ]
    for c in candidates:
        if c.exists():
            return c
    print("[Error] No recomp binary found in out/build/. Run 'just build' first.", file=sys.stderr)
    sys.exit(1)

def cmd_frontier(args):
    recomp_bin = find_recomp_bin()
    frames = args[0] if args else "1"
    cmd = [str(recomp_bin), "--headless", "--frames", str(frames), "--frontier", "--coverage"]
    print(f"[*] Inspecting Execution Frontier & Boot Gates ({frames} frame(s))...")
    proc = subprocess.Popen(
        cmd,
        cwd=str(REPO_ROOT),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )
    if proc.stdout:
        for line in proc.stdout:
            print(line, end="", flush=True)
    proc.wait()
    return proc.returncode

def cmd_trace(args):
    frames = args[0] if args else "5"
    recomp_bin = find_recomp_bin()
    cmd = [str(recomp_bin), "--headless", "--frames", str(frames), "--coverage"]
    print(f"[*] Running trace execution ({frames} frames)...")
    res = subprocess.run(cmd, cwd=str(REPO_ROOT), capture_output=True, text=True)

    # Parse unresolved ICALLs from both stdout and stderr
    combined = (res.stdout + "\n" + res.stderr).replace(r"\n", "\n")
    unresolved = {}
    pattern = re.compile(r"ICALL:\s+unresolved\s+VA\s+(0x[0-9A-Fa-f]+)\s+from\s+(0x[0-9A-Fa-f]+)")
    for line in combined.splitlines():
        for m in pattern.finditer(line):
            target = parse_va(m.group(1))
            caller = parse_va(m.group(2))
            if target not in unresolved:
                unresolved[target] = {"count": 0, "callers": set()}
            unresolved[target]["count"] += 1
            unresolved[target]["callers"].add(caller)

    # Load known function database if exists
    known_funcs = {}
    if FUNCTIONS_JSON.exists():
        try:
            with open(FUNCTIONS_JSON, "r") as f:
                data = json.load(f)
                for fn in data.get("functions", []):
                    known_funcs[int(fn["address"])] = fn
        except Exception:
            pass

    print("\n" + "=" * 78)
    print(f"  Unresolved Runtime Call Frontier ({len(unresolved)} unique targets)")
    print("=" * 78)
    print(f"  {'Target VA':<12} {'Hits':<6} {'Size':<8} {'Discovered Name / Callers'}")
    print("-" * 78)

    exe_path = find_game_exe()
    sorted_targets = sorted(unresolved.items(), key=lambda x: x[1]["count"], reverse=True)
    for target, meta in sorted_targets:
        fn_meta = known_funcs.get(target)
        if fn_meta:
            size_str = f"{fn_meta['size']} B"
            name_str = fn_meta.get("name", "")
        else:
            bin_info = analyze_function_binary(exe_path, target)
            if bin_info:
                size_str = f"{bin_info['size']} B"
                name_str = f"{bin_info['convention']}({bin_info['stack_purge']})"
            else:
                size_str = "unknown"
                name_str = ""
        callers_str = ", ".join(f"0x{c:08X}" for c in sorted(meta["callers"]))
        detail = f"{name_str} (Callers: {callers_str})" if name_str else f"Callers: {callers_str}"
        print(f"  0x{target:08X}   {meta['count']:<6} {size_str:<8} {detail}")

    print("=" * 78)

def cmd_auto_add(args):
    if not args:
        print("Usage: recomp_tool.py auto-add <address> [<name>]")
        return
    va = parse_va(args[0])
    name = args[1] if len(args) > 1 else f"func_{va:08X}"
    exe_path = find_game_exe()

    info = analyze_function_binary(exe_path, va)
    if not info:
        print(f"[Error] Could not analyze function at 0x{va:08X}")
        return

    # Check if already present in functions.toml
    content = FUNCTIONS_TOML.read_text(encoding="utf-8")
    if f"0x{va:08X}" in content or f"0x{va:x}" in content.lower():
        print(f"[*] Function 0x{va:08X} already present in functions.toml.")
        return

    toml_block = f"""
[[function]]
rva = 0x{va:08X}
name = "{name}"
size = {info['size']}
convention = "{info['convention']}"
stack_purge = {info['stack_purge']}
confidence = "high"
lift = true
evidence = ["Discovered via runtime trace and automated boundary analysis (size: {info['size']} bytes)"]
"""
    with open(FUNCTIONS_TOML, "a", encoding="utf-8") as f:
        f.write(toml_block)

    print(f"[+] Added 0x{va:08X} ({name}, {info['convention']}, purge={info['stack_purge']}, size={info['size']}B) to functions.toml")

def cmd_lift(args):
    force = "--force" in args
    # Check cache
    hasher = hashlib.sha256()
    inputs = [REPO_ROOT / "config" / "target.toml", find_game_exe()]
    inputs += sorted(CONFIG_DIR.glob("*.toml"))
    inputs += sorted((REPO_ROOT / "scripts" / "analysis").rglob("*.py"))
    for p in inputs:
        if p.exists():
            hasher.update(str(p.relative_to(REPO_ROOT)).encode())
            hasher.update(p.read_bytes())
    curr_hash = hasher.hexdigest()

    if not force and CACHE_FILE.exists():
        if CACHE_FILE.read_text().strip() == curr_hash:
            print("[*] Lifter cache is up-to-date. Skipping code regeneration (use --force to override).")
            return 0

    print("[*] Running static lifter...")
    py_exe = REPO_ROOT / ".venv" / "bin" / "python3"
    lift_script = REPO_ROOT / "scripts" / "analysis" / "run_lift.py"
    res = subprocess.run([
        str(py_exe), str(lift_script),
        "--revision", str(CONFIG_DIR),
        "--exe", str(find_game_exe())
    ], cwd=str(REPO_ROOT))
    if res.returncode == 0:
        CACHE_FILE.parent.mkdir(parents=True, exist_ok=True)
        CACHE_FILE.write_text(curr_hash)
    return res.returncode

def cmd_dev(args):
    # 1. Lift if changed
    lift_res = cmd_lift(args)
    if lift_res != 0:
        return lift_res

    # 2. Build with Ninja
    print("[*] Building project with Ninja...")
    build_res = subprocess.run(["ninja", "-C", "out/build/linux-amd64-release"], cwd=str(REPO_ROOT))
    if build_res.returncode != 0:
        return build_res.returncode

    # 3. Run unit tests
    print("[*] Running unit test suite...")
    py_exe = REPO_ROOT / ".venv" / "bin" / "python3"
    test_res = subprocess.run([str(py_exe), "-m", "unittest", "discover", "-s", "tests", "-p", "test_*.py"], cwd=str(REPO_ROOT))
    return test_res.returncode

def cmd_cycle(args):
    dev_res = cmd_dev(args)
    if dev_res != 0:
        return dev_res
    print("[*] Running execution trace...")
    cmd_trace(["5"])
    return 0

def cmd_run(args):
    """Launch the configured target, forwarding any runtime arguments."""
    return subprocess.run([str(find_recomp_bin()), *args], cwd=str(REPO_ROOT)).returncode

def main():
    if len(sys.argv) < 2:
        print("Usage: recomp_tool.py <command> [args...]")
        print("Commands:")
        print("  inspect <address>      Disassemble and analyze function boundaries/ABI")
        print("  trace [frames=5]       Trace recompiled runtime and list unresolved ICALLs")
        print("  frontier               Inspect Execution Frontier and Boot Gate progression")
        print("  trace-boot [frames=1]  Trace execution through PE entry and CRT startup")
        print("  trace-imports [frames] Trace Win32 API import resolutions")
        print("  trace-icalls [frames]  Trace indirect call dispatches")
        print("  trace-files [frames=1] Trace Win32 file I/O operations")
        print("  trace-threads [frames] Trace thread and synchronization operations")
        print("  auto-add <addr> [name] Add function to functions.toml with auto-detected metrics")
        print("  lift [--force]         Run static lifter with caching")
        print("  dev                    Incremental lift + ninja build + unit tests")
        print("  cycle                  Full pipeline (lift + build + test + trace)")
        print("  run [runtime args...]  Run the configured recompilation target")
        sys.exit(1)

    cmd = sys.argv[1]
    args = sys.argv[2:]

    dispatch = {
        "inspect": cmd_inspect,
        "trace": cmd_trace,
        "frontier": cmd_frontier,
        "trace-boot": lambda a: cmd_trace_flag("--trace-boot", a),
        "trace-imports": lambda a: cmd_trace_flag("--trace-imports", a),
        "trace-icalls": lambda a: cmd_trace_flag("--trace-icalls", a),
        "trace-files": lambda a: cmd_trace_flag("--trace-files", a),
        "trace-threads": lambda a: cmd_trace_flag("--trace-threads", a),
        "auto-add": cmd_auto_add,
        "lift": cmd_lift,
        "dev": cmd_dev,
        "cycle": cmd_cycle,
        "run": cmd_run,
    }

    if cmd in dispatch:
        ret = dispatch[cmd](args)
        sys.exit(ret if isinstance(ret, int) else 0)
    else:
        print(f"Unknown command '{cmd}'", file=sys.stderr)
        sys.exit(1)

def cmd_trace_flag(flag: str, args):
    recomp_bin = find_recomp_bin()
    frames = args[0] if args else "1"
    cmd = [str(recomp_bin), "--headless", "--frames", str(frames), flag]
    print(f"[*] Running execution with {flag} ({frames} frame(s))...")
    proc = subprocess.Popen(
        cmd,
        cwd=str(REPO_ROOT),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )
    if proc.stdout:
        for line in proc.stdout:
            print(line, end="", flush=True)
    proc.wait()
    return proc.returncode

if __name__ == "__main__":
    main()
