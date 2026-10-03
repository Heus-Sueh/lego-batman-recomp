# Override tools with: just cmake=/path/to/cmake python=python3 build
set windows-shell := ["cmd.exe", "/c"]

cmake := "cmake"
python := if path_exists(".venv/bin/python3") == "true" { ".venv/bin/python3" } else { "python3" }
preset := if os() == "windows" { "win-amd64-release" } else { "linux-amd64-release" }
target_config := "config/target.toml"

# Show available tasks.
default:
    @just --list

# Configure CMake with the active preset.
configure:
    "{{ cmake }}" --preset {{ preset }}

# Build the recompilation runtime and generated code.
build:
    "{{ cmake }}" --build --preset {{ preset }} --parallel

# Run the recompiled executable.
run:
    "{{ python }}" scripts/tools/recomp_tool.py run

# Run PE analysis on the user's selected executable.
pe-analyze exe_path:
    "{{ python }}" thirdparty/pcrecomp/tools/pe/pe_analyze.py "{{ exe_path }}" --json pe_analysis.json

# Analyze sections and copy-protection of the executable.
pe-sections exe_path:
    "{{ python }}" thirdparty/pcrecomp/tools/pe/analyze_sections.py "{{ exe_path }}"

# Extract imports and stack purge details.
pe-imports exe_path:
    "{{ python }}" thirdparty/pcrecomp/tools/pe/extract_imports.py "{{ exe_path }}"

# Disassemble 32-bit x86 code and discover functions.
disasm exe_path pe_json="pe_analysis.json" output="functions.json":
    "{{ python }}" thirdparty/pcrecomp/tools/disasm/disasm32.py "{{ exe_path }}" --pe-json "{{ pe_json }}" --output "{{ output }}"

# Lift targeted functions to C/C++.
lift output="generated/recomp":
    "{{ python }}" scripts/analysis/run_lift.py --target-config "{{ target_config }}" --output "{{ output }}"

# Run test suite without requiring retail game assets.
test:
    "{{ python }}" -m unittest discover -s tests -p "test_*.py"

# Standalone runtime libraries and tests: no SDL, DXVK or retail files.
test-runtime:
    "{{ cmake }}" -S . -B out/build/runtime-foundation -G Ninja -DPCRECOMP_BUILD_GAME=OFF -DBUILD_TESTING=ON
    "{{ cmake }}" --build out/build/runtime-foundation --parallel
    ctest --test-dir out/build/runtime-foundation --output-on-failure

# Inspect function boundary and disassemble at guest VA.
inspect addr:
    "{{ python }}" scripts/tools/recomp_tool.py inspect "{{ addr }}"

# Trace runtime execution and display unresolved call frontier.
trace frames="5":
    "{{ python }}" scripts/tools/recomp_tool.py trace "{{ frames }}"

# Automatically add discovered function to functions.toml.
auto-add addr name="":
    "{{ python }}" scripts/tools/recomp_tool.py auto-add "{{ addr }}" "{{ name }}"

# Rapid development: incremental lift + parallel ninja build + unit tests.
dev:
    "{{ python }}" scripts/tools/recomp_tool.py dev

# Inspect current Execution Frontier and Boot Gate progression.
frontier:
    "{{ python }}" scripts/tools/recomp_tool.py frontier

# Trace execution through original PE entry point and CRT startup.
trace-boot frames="1":
    "{{ python }}" scripts/tools/recomp_tool.py trace-boot "{{ frames }}"

# Trace Win32 API import invocations and resolutions.
trace-imports frames="1":
    "{{ python }}" scripts/tools/recomp_tool.py trace-imports "{{ frames }}"

# Trace indirect call dispatches.
trace-icalls frames="5":
    "{{ python }}" scripts/tools/recomp_tool.py trace-icalls "{{ frames }}"

# Trace Win32 file I/O operations and path mappings.
trace-files frames="1":
    "{{ python }}" scripts/tools/recomp_tool.py trace-files "{{ frames }}"

# Trace thread and synchronization operations.
trace-threads frames="1":
    "{{ python }}" scripts/tools/recomp_tool.py trace-threads "{{ frames }}"

# Full dev cycle: lift + build + test + trace unlifted call frontier.
cycle:
    "{{ python }}" scripts/tools/recomp_tool.py cycle
