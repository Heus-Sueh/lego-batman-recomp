"""Binary analysis, instruction disassembly, and calling convention / stack purge discovery."""

import json
import hashlib
from pathlib import Path
from .common import get_shared_cs, Cs

try:
    import pefile
except ImportError:
    pefile = None


def discover_stack_purge(target_va: int, code_data: bytes, code_start: int,
                         image_base: int, depth: int = 0, md: Cs | None = None) -> int | None:
    """Recover callee cleanup from a PE32 function's reachable epilogues.

    This intentionally reports only direct evidence from ``ret``/``ret N``.
    Conditional paths are followed conservatively and unresolved indirect
    control flow returns ``None`` instead of silently assuming cdecl.
    """
    if depth > 8 or not (code_start <= target_va < code_start + len(code_data)):
        return None
    if md is None:
        md = get_shared_cs()
    pending = [target_va]
    visited = set()
    while pending:
        va = pending.pop()
        if va in visited or not (code_start <= va < code_start + len(code_data)):
            continue
        visited.add(va)
        offset = va - code_start
        for ins in md.disasm(code_data[offset:offset + 0x1000], va):
            if ins.mnemonic == "ret":
                return ins.operands[0].imm if ins.operands else 0
            if ins.mnemonic in ("jmp", "je", "jne", "ja", "jb", "jae", "jbe", "jg", "jl", "jge", "jle", "jo", "jno", "js", "jns", "jp", "jnp"):
                if ins.operands and ins.operands[0].type == 2:  # X86_OP_IMM
                    pending.append(ins.operands[0].imm)
                if ins.mnemonic == "jmp":
                    break
            if ins.mnemonic in ("call", "int", "ud2"):
                continue
    return None


def discover_direct_callees(exe_path: Path, functions: list[dict], cache_dir: Path | None = None) -> dict[int, int]:
    """Find direct call/tail-jump targets and their proven stack purges."""
    if pefile is None or Cs is None:
        return {}

    cache_file = (cache_dir / ".callees_cache.json") if cache_dir else None
    cache_key = None
    if cache_file and exe_path.exists():
        try:
            stat = exe_path.stat()
            func_rvas = sorted(fn["rva"] for fn in functions if fn.get("lift", True) is not False)
            key_str = f"{stat.st_mtime}_{stat.st_size}_{len(func_rvas)}_{func_rvas[:10]}_{func_rvas[-10:]}"
            cache_key = hashlib.sha256(key_str.encode("utf-8")).hexdigest()
            if cache_file.exists():
                with open(cache_file, "r", encoding="utf-8") as f:
                    cached = json.load(f)
                if cached.get("key") == cache_key:
                    return {int(k): v for k, v in cached.get("discovered", {}).items()}
        except Exception:
            pass

    pe = pefile.PE(str(exe_path), fast_load=True)
    image_base = pe.OPTIONAL_HEADER.ImageBase
    text = next((s for s in pe.sections if s.Name.rstrip(b"\0") in (b".text", b"CODE")), None)
    if text is None:
        return {}
    code_start = image_base + text.VirtualAddress
    code_data = text.get_data()
    md = get_shared_cs()
    known = {fn["rva"] for fn in functions}
    discovered = {}
    for fn in functions:
        if fn.get("lift", True) is False:
            continue
        start = fn["rva"] - code_start
        if not (0 <= start < len(code_data)):
            continue
        for ins in md.disasm(code_data[start:start + int(fn.get("size", 0x1000))], fn["rva"]):
            if ins.mnemonic not in ("call", "jmp") or not ins.operands or ins.operands[0].type != 2:
                continue
            target = ins.operands[0].imm
            if target in known or not (code_start <= target < code_start + len(code_data)):
                continue
            purge = discover_stack_purge(target, code_data, code_start, image_base, md=md)
            if purge is not None:
                discovered[target] = purge

    if cache_file and cache_key:
        try:
            with open(cache_file, "w", encoding="utf-8") as f:
                json.dump({"key": cache_key, "discovered": discovered}, f)
        except Exception:
            pass

    return discovered
