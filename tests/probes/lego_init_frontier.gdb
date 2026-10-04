# Requires the matching user-provided binary and regenerated release executable.
# Run: gdb -q -batch -x tests/probes/lego_init_frontier.gdb --args out/build/linux-amd64-release/legobatman
set pagination off
set confirm off
set debuginfod enabled off
init-if-undefined $expected_cleanup = 4
break recomp_func_006D5510
run --headless --frames 1
set $before = (unsigned int)::g_esp
set $base = (unsigned long long)::g_mem_base
set $arg1 = *(unsigned int*)($base + $before + 4)
set $arg2 = *(unsigned int*)($base + $before + 8)
set $value1 = *(unsigned int*)($base + $arg1)
set $value2 = *(unsigned int*)($base + $arg2)
finish
if (unsigned int)::g_esp != $before + $expected_cleanup || *(unsigned int*)($base + 0xb028e0) != $value1 || *(unsigned int*)($base + 0xb028e4) != $value2
    echo FAIL: pair store or stack cleanup\n
    quit 1
end
echo PASS: pair store and stack cleanup\n
break recomp_func_005C8110
continue
set $before = (unsigned int)::g_esp
finish
if (unsigned int)::g_esp != $before + 4
    echo FAIL: clear region stack cleanup\n
    quit 1
end
python
import gdb
base = int(gdb.parse_and_eval('$base'))
data = gdb.selected_inferior().read_memory(base + 0xab29e8, 0xa790)
if any(bytes(data)):
    gdb.write('FAIL: clear region contents\n')
    gdb.execute('quit 1')
end
echo PASS: clear region contents and stack cleanup\n
quit 0
