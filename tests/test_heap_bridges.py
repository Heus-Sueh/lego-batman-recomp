"""Execute freshly generated heap bridges against a synthetic guest arena."""
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts/analysis"))
from lifter.dispatch_gen import generate_recomp_dispatch_cpp


class HeapBridgeTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("c++"), "C++ compiler required")
    def test_generated_stdcall_and_guest_offsets(self):
        with tempfile.TemporaryDirectory() as directory:
            tmp = Path(directory)
            generate_recomp_dispatch_cpp(tmp, [])
            source = (tmp / "recomp_dispatch.cpp").read_text()
            names = ["GetProcessHeap", "HeapCreate", "HeapAlloc", "HeapSize",
                     "HeapReAlloc", "HeapFree", "HeapDestroy"]
            bridges = []
            for name in names:
                match = re.search(r"static void bridge_import_" + name +
                                  r"\(void\) \{.*?\n\}", source, re.S)
                self.assertIsNotNone(match, name)
                bridges.append(match.group())
            harness = r'''
#include <win32compat/heap.h>
#include <cstdint>
#include <cstdlib>
alignas(16) uint8_t arena[4096]{};
uint32_t eax, esp, stack[32];
#define MEM32(a) stack[(a)/4]
#define CHECK(x) do { if (!(x)) std::abort(); } while (0)
namespace pcrecomp::runtime {
win32compat::GuestHeap& guest_heap() {
    static win32compat::GuestHeap heap(arena, 16, sizeof(arena));
    return heap;
}}
''' + '\n'.join(bridges) + r'''
int main() {
    esp = 0; bridge_import_GetProcessHeap();
    CHECK(esp == 4 && eax == win32compat::GuestHeap::process_heap);
    esp = 0; stack[1] = 0; stack[2] = 0; stack[3] = 0;
    bridge_import_HeapCreate(); CHECK(esp == 16 && eax);
    const auto heap = eax;
    esp = 0; stack[1] = heap; stack[2] = 8; stack[3] = 17;
    bridge_import_HeapAlloc(); CHECK(esp == 16 && eax == 16);
    const auto block = eax; arena[block] = 42;
    esp = 0; stack[3] = block;
    bridge_import_HeapSize(); CHECK(esp == 16 && eax == 17);
    esp = 0; stack[4] = 80;
    bridge_import_HeapReAlloc(); CHECK(esp == 20 && eax && arena[eax] == 42);
    esp = 0; stack[3] = eax;
    bridge_import_HeapFree(); CHECK(esp == 16 && eax == 1);
    esp = 0; bridge_import_HeapDestroy(); CHECK(esp == 8 && eax == 1);
}
'''
            cpp = tmp / "bridges.cpp"
            cpp.write_text(harness)
            binary = tmp / "bridges"
            subprocess.run(["c++", "-std=c++17", "-pthread", "-I",
                            str(ROOT / "win32compat/include"), str(cpp),
                            str(ROOT / "win32compat/src/heap.cpp"), "-o", str(binary)],
                           check=True, capture_output=True)
            subprocess.run([str(binary)], check=True)
