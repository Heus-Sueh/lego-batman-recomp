"""Authentic x86 instruction lifting, CFG recovery, deterministic patches, and unit partitioning."""

import sys
import re
import struct
from pathlib import Path

TOOLS_LIFT = Path(__file__).resolve().parent.parent.parent.parent / "thirdparty" / "pcrecomp" / "tools" / "lift"
TOOLS_PE = Path(__file__).resolve().parent.parent.parent.parent / "thirdparty" / "pcrecomp" / "tools" / "pe"
if str(TOOLS_LIFT) not in sys.path:
    sys.path.insert(0, str(TOOLS_LIFT))
if str(TOOLS_PE) not in sys.path:
    sys.path.insert(0, str(TOOLS_PE))

import pefile
from pe_analyze import analyze_pe, build_iat_map
import lift32
from lift32 import Lifter
from generate import LinearInstruction, linear_disassemble_function, lift_function_linear
from .common import HEADER_COMMENT, write_if_changed, get_shared_cs, Cs

def generate_game_cpp(out_dir: Path, exe_path: Path = None, functions: list[dict] = None, target: dict = None):
    prefix = "legobatman"
    if target:
        prefix = target.get("output_name") or target.get("id") or "legobatman"

    if exe_path is None:
        if target:
            exe_path = Path(target.get("default_game_dir", "game")) / target.get("guest_executable", "LEGOBatman.exe")
        else:
            exe_path = Path("game/LEGOBatman.exe")

    if exe_path and exe_path.exists() and Lifter is not None and pefile is not None:
        print(f"[*] Lifting authentic game & menu logic from {exe_path}...")
        pe = pefile.PE(str(exe_path))
        text_sec = [s for s in pe.sections if s.Name.startswith(b".text")][0]
        code_data = text_sec.get_data()
        code_start = pe.OPTIONAL_HEADER.ImageBase + text_sec.VirtualAddress

        pe_info = analyze_pe(str(exe_path))
        iat_map = build_iat_map(pe_info)

        md = get_shared_cs()

        all_funcs = functions or []
        func_bounds = {fn["rva"]: fn["rva"] + fn["size"] for fn in all_funcs if fn.get("size")}
        func_leaders = {fn["rva"]: set(fn["leaders"]) for fn in all_funcs if "leaders" in fn}

        def recover_jump_tables(insns, fn_start, max_fn_end):
            tables = []
            for i, li in enumerate(insns):
                if li.mnemonic != "jmp" or not li.operands:
                    continue
                op = li.operands[0]
                if op.type != 3:  # X86_OP_MEM
                    continue
                mem = op.mem
                if mem.scale != 4 or mem.disp == 0:
                    continue
                table_va = mem.disp
                reg = md.reg_name(mem.index)
                default_target = None
                count_from_cmp = None
                if i > 0 and insns[i - 1].mnemonic in ("ja", "jae"):
                    prev = insns[i - 1]
                    if prev.operands and prev.operands[0].type == 2:  # X86_OP_IMM
                        default_target = prev.operands[0].imm
                    if i > 1 and insns[i - 2].mnemonic == "cmp":
                        cmp_ins = insns[i - 2]
                        if len(cmp_ins.operands) == 2 and cmp_ins.operands[1].type == 2:
                            max_idx = cmp_ins.operands[1].imm
                            count_from_cmp = max_idx + 1 if prev.mnemonic == "ja" else max_idx
                elif i > 1 and insns[i - 2].mnemonic in ("ja", "jae"):
                    prev = insns[i - 2]
                    if prev.operands and prev.operands[0].type == 2:  # X86_OP_IMM
                        default_target = prev.operands[0].imm
                    if i > 2 and insns[i - 3].mnemonic == "cmp":
                        cmp_ins = insns[i - 3]
                        if len(cmp_ins.operands) == 2 and cmp_ins.operands[1].type == 2:
                            max_idx = cmp_ins.operands[1].imm
                            count_from_cmp = max_idx + 1 if prev.mnemonic == "ja" else max_idx

                num_to_read = count_from_cmp if count_from_cmp and 0 < count_from_cmp <= 256 else 256
                targets = []
                for c in range(num_to_read):
                    t_va = table_va + c * 4
                    rva = t_va - pe.OPTIONAL_HEADER.ImageBase
                    try:
                        d = pe.get_data(rva, 4)
                        if len(d) < 4:
                            break
                        tgt = struct.unpack("<I", d)[0]
                        if fn_start <= tgt < max_fn_end:
                            targets.append(tgt)
                        else:
                            break
                    except Exception:
                        break
                if targets:
                    tables.append({
                        "ins_addr": li.address,
                        "reg": reg,
                        "table_va": table_va,
                        "targets": targets,
                        "default_target": default_target
                    })
            return tables

        def build_switch_replacement(table_info, is_winmain=False):
            reg = table_info["reg"]
            targets = table_info["targets"]
            default_target = table_info["default_target"]
            target_to_cases = {}
            for idx, tgt in enumerate(targets):
                target_to_cases.setdefault(tgt, []).append(idx)

            lines = []
            if is_winmain:
                lines.append('static uint32_t s_last_state = 999; if (eax != s_last_state) { std::cout << "[WinMain State] " << eax << std::endl; s_last_state = eax; }')

            lines.append(f"switch ({reg}) {{")
            for tgt, cases in sorted(target_to_cases.items(), key=lambda item: item[1][0]):
                case_strs = " ".join(f"case {c}:" for c in cases)
                lines.append(f" {case_strs} goto L_{tgt:08X};")
            if default_target:
                lines.append(f" default: goto L_{default_target:08X};")
            else:
                last_tgt = targets[-1]
                lines.append(f" default: goto L_{last_tgt:08X};")
            lines.append("}")
            return "".join(lines)

        def auto_disasm(va):
            offset = va - code_start
            raw = code_data[offset:offset + 65536]
            insns = []
            leaders = {va}
            max_target = va
            max_end = func_bounds.get(va)
            for insn in md.disasm(raw, va):
                li = LinearInstruction(insn)
                insns.append(li)
                if li.is_jump:
                    tgt = li.get_branch_target()
                    if tgt and tgt >= va and tgt < va + 65536:
                        leaders.add(tgt)
                        if tgt > max_target:
                            max_target = tgt
                if max_end and li.end_address >= max_end:
                    break
                if not max_end and li.end_address > max_target and (li.is_ret or li.mnemonic == "int3" or (li.is_uncond_jump and li.get_branch_target() and li.get_branch_target() < va)):
                    break

            tables = recover_jump_tables(insns, va, max_end or (va + 65536))
            for t in tables:
                for tgt in t["targets"]:
                    leaders.add(tgt)
                    if tgt > max_target:
                        max_target = tgt
                if t["default_target"] and t["default_target"] < (max_end or va + 65536):
                    leaders.add(t["default_target"])
                    if t["default_target"] > max_target:
                        max_target = t["default_target"]
            if va in func_leaders:
                leaders.update(func_leaders[va])

            if va == 0x00748710:
                leaders.update([0x0074886C, 0x00748751, 0x00748745])
            elif va == 0x0053EC10:
                leaders.update([0x0053EC24])
            elif va == 0x0053E920:
                leaders.update([0x0053E930, 0x0053EA3F, 0x0053EA7D, 0x0053EAE6, 0x0053EB08, 0x0053EB19, 0x0053EB7E, 0x0053EB91, 0x0053EBCD, 0x0053EBE4, 0x0053EC09])
            elif va == 0x00541DD0:
                leaders.update([0x00541DD7])
            elif va == 0x00541C40:
                leaders.update([0x00541D1F])
            elif va == 0x0052F590:
                leaders.update([0x0052F5A2])
            elif va == 0x0053E770:
                leaders.update([0x0053E7A0, 0x0053E832, 0x0053E867, 0x0053E890])
            elif va == 0x0057C290:
                leaders.update([0x0057C2DD, 0x0057C2FD, 0x0057C321, 0x0057C329])
            elif va == 0x00727240:
                leaders.update([0x00727258])
            elif va == 0x0058FF60:
                leaders.update([0x00590004, 0x00590062, 0x00590085, 0x0059008F, 0x005900A1])
            elif va == 0x00727420:
                leaders.update([0x00727420])
            elif va == 0x00728350:
                leaders.update([0x00728350, 0x00728375, 0x0072837D, 0x0072838A])
            elif va == 0x00727B60:
                leaders.update([0x00727B60, 0x00727B65, 0x00727B99, 0x00727BA5])
            elif va == 0x00531F20:
                leaders.update([0x00531F27, 0x00531F30, 0x00531FAD])
            elif va == 0x0053E580:
                leaders.update([0x0053E5B0])

            return insns, leaders, tables

        # Target list of authentic functions to lift (driven declaratively by functions.toml)
        bridge_vas = {fn["rva"] for fn in all_funcs if fn.get("bridge")}
        vas = [fn["rva"] for fn in all_funcs if fn.get("lift") is True and fn["rva"] not in bridge_vas]
        if not vas:
            vas = [0x00824570, 0x008286E0, 0x00822070, 0x00748710]

        func_names = {f["rva"]: f"func_{f['rva']:08X}" for f in all_funcs if "rva" in f}
        for va in vas:
            func_names[va] = f"func_{va:08X}"

        lifter = Lifter(iat_map=iat_map, func_names=func_names, lifted=set(vas))

        file_header = HEADER_COMMENT + """
#define RECOMP_GENERATED_CODE
#include "../../thirdparty/pcrecomp/runtime/recomp32/recomp_types.h"
#include "recomp_funcs.h"
#include <iostream>
#include <cstring>

extern "C" {
extern int32_t g_max_frames;
extern uint32_t g_frame_counter;
"""
        fwd_decls = "\n".join(f"void recomp_func_{va:08X}(void);" for va in vas)

        # Lift each authentic function
        lifted_code_map = {}
        for va in vas:
            insns, leaders, tables = auto_disasm(va)
            c_code = lift_function_linear(lifter, f"recomp_func_{va:08X}", insns, leaders, va)

            # Automatically replace recovered switch jump tables with native C++ switch statements
            for t in tables:
                switch_c = build_switch_replacement(t, is_winmain=(va == 0x00748710))
                tbl_hex = f"{t['table_va']:X}"
                reg_name = t["reg"]
                pattern = re.compile(
                    r'\{\s*_itail_tgt\s*=\s*\(uint32_t\)\(MEM32\(' + re.escape(reg_name) + r'\s*\*\s*4\s*\+\s*0x' + tbl_hex + r'\)\);\s*goto\s*_ljump;\s*\}',
                    flags=re.IGNORECASE
                )
                c_code = pattern.sub(switch_c, c_code)

            if va == 0x00748710:
                c_code = c_code.replace("RECOMP_ICALL(ebp); /* 0x00748EC0", "ebp = MEM32(0x85826C);\n    RECOMP_ICALL(ebp); /* 0x00748EC0")
                c_code = c_code.replace("L_00748DC9:", "L_00748DC9: static uint32_t s_loop_cnt = 0; if ((s_loop_cnt++ % 500) == 0) std::cout << \"[LOOP_CHECK] iter=\" << s_loop_cnt << \" quit=\" << MEM32(0xC17050) << \" eax=\" << eax << \" state=\" << MEM32(0xC8D4C0) << std::endl;")
                c_code = c_code.replace("L_00748DA3:", "L_00748DA3: ebx = 0; edi = 1;")
                c_code = c_code.replace("L_00748A41:", "L_00748A41: ebx = 0; edi = 1;")
                c_code = c_code.replace("L_00748D1F:", "L_00748D1F: edi = 1;")
            elif va == 0x0053EC10:
                c_code = c_code.replace("L_0053EC10:", "L_0053EC10: static uint32_t s_ev_cnt = 0; if ((s_ev_cnt++ % 500) == 0) std::cout << \"[53EC10 Event] ev=\" << MEM32(esp + 4) << \" count=\" << s_ev_cnt << std::endl;")
                c_code = c_code.replace("L_0053ECB8:", "L_0053ECB8: static uint32_t s_b8_cnt = 0; if (s_b8_cnt++ < 5) std::cout << \"[53EC10] L_0053ECB8 reached! calling 0x53E920\" << std::endl;")
                c_code = c_code.replace("L_0053ECD6:", "L_0053ECD6: static uint32_t s_d6_cnt = 0; if (s_d6_cnt++ < 5) std::cout << \"[53EC10] L_0053ECD6 (unhandled) reached! eax=\" << eax << std::endl;")
                c_code = re.sub(
                    r'goto\s+L_004018CE;.*',
                    'eax = (uint32_t)(MEM8(eax + 0x0053ED08u)); goto L_0053EC24;',
                    c_code
                )
            elif va == 0x0053E920:
                c_code = c_code.replace(
                    "L_0053E920:",
                    "L_0053E920: static uint32_t s_idle_cnt = 0; if (s_idle_cnt++ < 5) std::cout << \"[53E920] Idle entered (\" << s_idle_cnt << \") arg=\" << MEM32(esp + 4) << std::endl;"
                )
                c_code = c_code.replace(
                    "RECOMP_CALL(recomp_func_0053BEE0);",
                    "static uint32_t s_proc_cnt = 0; if (s_proc_cnt++ < 5) std::cout << \"[53E920] calling CGame::Process (0x53BEE0)\" << std::endl; RECOMP_CALL(recomp_func_0053BEE0);"
                )
                c_code = c_code.replace(
                    "RECOMP_ICALL(0x00619440u);",
                    "static uint32_t s_show_cnt = 0; if (s_show_cnt++ < 5) std::cout << \"[53E920] calling RsCameraShowRaster (0x619440)\" << std::endl; RECOMP_ICALL(0x00619440u);"
                )
                c_code = c_code.replace(
                    "L_0053EC09:",
                    "L_0053EC09: static uint32_t s_exit_cnt = 0; if (s_exit_cnt++ < 5) std::cout << \"[53E920] exit L_0053EC09 reached eax=\" << eax << std::endl;"
                )
            elif va == 0x00590990:
                c_code = c_code.replace("esi = 0xC8u;", "esi = 2;")
                c_code = c_code.replace("esi = 0x32u;", "esi = 2;")
            elif va == 0x005733E0:
                c_code = """void recomp_func_005733E0(void) {
    RECOMP_ENTER(0x005733E0u);
    float val = *(float*)ADDR(esp + 4);
    uint32_t width = MEM32(0xC17044);
    if (width == 640) {
        fp_push((double)val);
    } else {
        fp_push((double)width * (double)val * (double)*(float*)ADDR(0x859520));
    }
    esp += 8;
}
"""
            elif va == 0x00824570:
                c_code = re.sub(
                    r'RECOMP_CALL\(recomp_func_00748710\);[\s\S]*?MEM32\(ebp \+ \(-0x6C\)\) = edi;',
                    'RECOMP_CALL(recomp_func_00748710);\\n    edi = eax;\\n    return;',
                    c_code
                )
            elif va == 0x00541DD0:
                c_code = re.sub(
                    r'goto\s+L_0040380D;.*',
                    'ecx = 0x00B73458u; goto L_00541DD7;',
                    c_code
                )
            elif va == 0x0052F590:
                c_code = re.sub(
                    r'goto\s+L_00401D4A;.*',
                    'ebx = MEM32(ecx + eax + 0xB70); goto L_0052F5A2;',
                    c_code
                )
            elif va == 0x00530640:
                c_code = re.sub(
                    r'goto\s+L_00409ABF;.*',
                    'PUSH32(esp, 0x3FBu);',
                    c_code
                )
            elif va == 0x00590860:
                c_code = re.sub(
                    r'goto\s+L_00406465;.*',
                    'MEM8(0x00BAB31E) = 1;',
                    c_code
                )
            elif va == 0x00728350:
                c_code = re.sub(
                    r'goto\s+L_00409B78;.*',
                    'ecx = MEM32(0x00C97B24u); goto L_00728375;',
                    c_code
                )
            elif va == 0x00727B60:
                c_code = re.sub(
                    r'goto\s+L_00409737;.*',
                    'eax = MEM32(0x00C97B24u); goto L_00727B65;',
                    c_code
                )
            elif va == 0x00727240:
                c_code = re.sub(
                    r'goto\s+L_00404875;.*',
                    'MEM32(esi) = 0; goto L_00727258;',
                    c_code
                )
            elif va == 0x00531F20:
                c_code = re.sub(
                    r'goto\s+L_00401182;.*',
                    'goto L_00531F30;',
                    c_code
                )

            lifted_code_map[va] = c_code

        # Collect generated filenames to clean up only obsolete files later
        new_generated_files = set()
        batch_size = 35
        batches = [vas[i:i + batch_size] for i in range(0, len(vas), batch_size)]

        for batch_idx, batch_vas in enumerate(batches):
            batch_lines = [
                file_header,
                fwd_decls,
                ""
            ]
            for va in batch_vas:
                batch_lines.append(lifted_code_map[va])
                batch_lines.append("")
            batch_lines.append("} // extern \"C\"\n")
            batch_filename = f"{prefix}_game_{batch_idx:03d}.cpp"
            new_generated_files.add(batch_filename)
            changed = write_if_changed(out_dir / batch_filename, "\n".join(batch_lines))
            status_str = "written" if changed else "up-to-date"
            print(f"[+] Lifted batch {batch_idx:03d} ({len(batch_vas)} functions) -> {out_dir / batch_filename} ({status_str})")

        # Compatibility bridges
        bridges = """} // extern \"C\"\n"""
        bridges_lines = [
            file_header,
            fwd_decls,
            "",
            bridges,
            ""
        ]
        bridges_filename = f"{prefix}_game_bridges.cpp"
        new_generated_files.add(bridges_filename)
        bridges_changed = write_if_changed(out_dir / bridges_filename, "\n".join(bridges_lines))
        status_str = "written" if bridges_changed else "up-to-date"
        print(f"[+] Emitted authentic compatibility bridges -> {out_dir / bridges_filename} ({status_str})")
        # Remove any stale generated files (including old gta_sa_game_*.cpp or other batches)
        for old_file in out_dir.glob("*.cpp"):
            if old_file.name not in new_generated_files and ("_game_" in old_file.name or old_file.name.startswith("gta_sa_")):
                try:
                    old_file.unlink()
                except OSError:
                    pass
        print(f"[+] Partitioned {len(vas)} authentic game functions across {len(batches)} compilation units + 1 bridge unit.")
        return


