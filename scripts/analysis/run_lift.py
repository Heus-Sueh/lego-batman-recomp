#!/usr/bin/env python3
"""Generate lifted C++ from the active target and revision manifests."""

import sys
import argparse
import tomllib
from pathlib import Path

# Add lifter package to path
LIFTER_DIR = Path(__file__).resolve().parent
if str(LIFTER_DIR) not in sys.path:
    sys.path.insert(0, str(LIFTER_DIR))

from lifter import (
    discover_direct_callees,
    generate_recomp_globals_cpp,
    generate_recomp_funcs_h,
    generate_recomp_dispatch_cpp,
    generate_crt_cpp,
    generate_winmain_cpp,
    generate_rw_cpp,
    generate_game_cpp,
)


def load_revision_data(rev_dir: Path):
    funcs_path = rev_dir / "functions.toml"
    globals_path = rev_dir / "globals.toml"

    if not funcs_path.exists():
        print(f"[Error] {funcs_path} does not exist.", file=sys.stderr)
        sys.exit(1)

    with open(funcs_path, "rb") as f:
        funcs = tomllib.load(f).get("function", [])

    globals_list = []
    if globals_path.exists():
        with open(globals_path, "rb") as f:
            globals_list = tomllib.load(f).get("global", [])

    return funcs, globals_list


def main():
    parser = argparse.ArgumentParser(description="Lift targeted functions into C++ recompilation sources.")
    parser.add_argument("--target-config", type=str, default="config/target.toml",
                        help="Path to the target manifest")
    parser.add_argument("--revision", "-r", type=str,
                        help="Path to executable revision configuration directory")
    parser.add_argument("--output", "-o", type=str, default="generated/recomp",
                        help="Output directory for generated C++ sources")
    parser.add_argument("--exe", "-e", type=str, help="Path to the original executable")
    args = parser.parse_args()

    with open(args.target_config, "rb") as f:
        target = tomllib.load(f)["target"]
    rev_dir = Path(args.revision or target["active_revision"])
    out_dir = Path(args.output)

    rev_toml = rev_dir / "revision.toml"
    exe_name = None
    if rev_toml.exists():
        with open(rev_toml, "rb") as f:
            rdata = tomllib.load(f)
            exe_name = rdata.get("executable", {}).get("name")

    if args.exe:
        exe_path = Path(args.exe)
    elif exe_name:
        candidates = [
            Path(target["default_game_dir"]) / exe_name,
            Path(exe_name),
        ]
        exe_path = None
        for cand in candidates:
            if cand.exists():
                exe_path = cand
                break
        if not exe_path:
            exe_path = Path("game") / exe_name
    else:
        exe_path = Path(target["default_game_dir"]) / target["guest_executable"]
    out_dir.mkdir(parents=True, exist_ok=True)

    print("=" * 70)
    print(f"  pcrecomp {target['title']} - Targeted Function Lifter")
    print(f"  Revision: {rev_dir}")
    print(f"  Output:   {out_dir}")
    print(f"  Binary:   {exe_path}")
    print("=" * 70)

    functions, globals_list = load_revision_data(rev_dir)
    print(f"[*] Loaded {len(functions)} function definitions.")
    auto_bridges = discover_direct_callees(exe_path, functions, cache_dir=out_dir)
    print(f"[*] Discovered {len(auto_bridges)} direct callees with inferred stack purge values.")
    if target.get("adapter") == "lego_batman":
        # ABI cleanup is not an implementation of the original function.
        # Keep missing guest code visible at the execution frontier.
        auto_bridges = {}
        print("[*] LEGO Batman: unresolved guest callees remain fatal; automatic return stubs disabled.")

    print("[*] Generating recomp_globals.cpp...")
    generate_recomp_globals_cpp(out_dir)

    print("[*] Generating recomp_funcs.h...")
    generate_recomp_funcs_h(out_dir, functions)

    print("[*] Generating recomp_dispatch.cpp...")
    generate_recomp_dispatch_cpp(out_dir, functions, auto_bridges=auto_bridges, exe_path=exe_path)

    print("[*] Generating function implementation units...")
    generate_game_cpp(out_dir, exe_path=exe_path, functions=functions)

    print("[+] Code generation complete. All files written to", out_dir)


if __name__ == "__main__":
    main()
