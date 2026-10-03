import sys
import unittest
import subprocess
import tomllib
from pathlib import Path

with open(Path(__file__).resolve().parent.parent / "config" / "target.toml", "rb") as f:
    ACTIVE_TARGET = tomllib.load(f)["target"]
legacy_only = unittest.skipUnless(ACTIVE_TARGET["adapter"] == "legacy_pc", "Legacy RenderWare diagnostic, not a LEGO Batman capability")


class TestRuntime(unittest.TestCase):
    _proc_cache = {}

    @classmethod
    def run_cmd(cls, cmd, use_cache=True):
        key = tuple(cmd)
        if use_cache and key in cls._proc_cache:
            return cls._proc_cache[key]
        res = subprocess.run(cmd, cwd=str(cls.repo_root), capture_output=True, text=True)
        if use_cache:
            cls._proc_cache[key] = res
        return res

    @classmethod
    def setUpClass(cls):
        cls.repo_root = Path(__file__).resolve().parent.parent
        with open(cls.repo_root / "config" / "target.toml", "rb") as f:
            target = tomllib.load(f)["target"]
        cls.exe_path = cls.repo_root / "out" / "build" / "linux-amd64-release" / target["output_name"]
        cls.log_path = cls.repo_root / "logs" / "unresolved_calls.log"

    def test_01_binary_exists(self):
        self.assertTrue(self.exe_path.exists(), f"Executable not found at {self.exe_path}")

    def test_02_headless_startup(self):
        cmd = [str(self.exe_path), "--headless", "--frames", "5"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Headless execution failed: {res.stderr}")
        self.assertIn("Entering original PE entry point: mainCRTStartup (0x0073D6A3)", res.stdout)

    def test_03_section_loading_and_imports(self):
        cmd = [str(self.exe_path), "--headless", "--test-imports"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Import verification failed: {res.stderr}")
        self.assertIn("Import shims resolved successfully", res.stdout)
        self.assertIn(".rdata populated: YES", res.stdout)
        self.assertIn(".data populated: YES", res.stdout)

    def test_04_unresolved_indirect_call_logging(self):
        if self.log_path.exists():
            self.log_path.unlink()

        cmd = [str(self.exe_path), "--headless", "--test-unresolved"]
        res = self.run_cmd(cmd, use_cache=False)
        self.assertEqual(res.returncode, 0)
        self.assertTrue(self.log_path.exists(), "unresolved_calls.log was not created")

        with open(self.log_path, "r", encoding="utf-8") as f:
            content = f.read()
        self.assertIn("0xdeadbeef", content)
        caller = "0x00748da0" if ACTIVE_TARGET["adapter"] == "legacy_pc" else "0x0073d6a3"
        self.assertIn("caller=" + caller, content)

    def test_05_recompiled_startup_dispatch(self):
        cmd = [str(self.exe_path), "--headless", "--test-dispatch"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Dispatch test failed: {res.stderr}")
        self.assertIn("Executing recompiled entry point dispatch (0x0073D6A3)", res.stdout)
        self.assertIn("Dispatch execution completed successfully", res.stdout)

    def test_06_headless_multi_frame_loop(self):
        cmd = [str(self.exe_path), "--headless", "--frames", "5"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Multi-frame test failed: {res.stderr}")
        self.assertIn("Entering original PE entry point: mainCRTStartup (0x0073D6A3)", res.stdout)

    def test_07_windowed_presentation_pacing(self):
        cmd = [str(self.exe_path), "--frames", "5"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Windowed presentation failed: {res.stderr}")
        self.assertIn("SDL3 initialized successfully", res.stdout)

    @legacy_only
    def test_08_custom_resolution_and_fps(self):
        cmd = [str(self.exe_path), "--headless", "--frames", "5", "--fps", "60", "--width", "1280", "--height", "720"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Custom FPS/res failed: {res.stderr}")
        self.assertIn("tick duration: 16666 us", res.stdout)

    @legacy_only
    def test_09_raster_resource_lifecycle(self):
        cmd = [str(self.exe_path), "--headless", "--test-raster"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Raster test failed: {res.stderr}")
        self.assertIn("Verifying RenderWare raster lifecycle (64x64 ARGB)", res.stdout)
        self.assertIn("Raster and texture created and unlocked successfully", res.stdout)
        self.assertIn("Raster and texture destroyed successfully", res.stdout)

    @legacy_only
    def test_10_splash_screen_presentation(self):
        cmd = [str(self.exe_path), "--test-splash", "--frames", "5"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Splash presentation failed: {res.stderr}")
        self.assertIn("Displaying loading screen splash", res.stdout)
        self.assertIn("Swapchain presentations: 5", res.stdout)

    @legacy_only
    def test_11_menu_initialization_and_state_machine(self):
        cmd = [str(self.exe_path), "--headless", "--test-menu"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Menu test failed: {res.stderr}")
        self.assertIn("Validating authentic CMenuManager state machine (0x00574F30)", res.stdout)
        self.assertIn("Authentic CMenuManager state machine validated successfully", res.stdout)

    @legacy_only
    def test_12_im3d_pipeline(self):
        cmd = [str(self.exe_path), "--headless", "--test-im3d"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"RwIm3D test failed: {res.stderr}")
        self.assertIn("Validating RenderWare 3D pipeline (RwIm3D)", res.stdout)
        self.assertIn("RenderWare 3D pipeline (RwIm3D) and perspective projection validated successfully", res.stdout)

    def test_13_authentic_game_asset_discovery(self):
        cmd = [str(self.exe_path), "--headless", "--frames", "5"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"World asset test failed: {res.stderr}")
        self.assertIn("Discovered game installation at:", res.stdout)

    def test_14_trace_boot_entry_point(self):
        cmd = [str(self.exe_path), "--headless", "--trace-boot"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Trace boot failed: {res.stderr}")
        self.assertIn("Entering original PE entry point: mainCRTStartup (0x0073D6A3)", res.stdout)
        self.assertIn("Original PE entry point execution completed successfully", res.stdout)

    def test_15_authoritative_main_loop_fl7_fl8(self):
        cmd = [str(self.exe_path), "--headless", "--frames", "5"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Original main loop test failed: {res.stderr}")
        self.assertIn("Entering original PE entry point: mainCRTStartup (0x0073D6A3)", res.stdout)

    def test_16_runtime_coverage_report(self):
        cmd = [str(self.exe_path), "--headless", "--frames", "5", "--coverage"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Coverage test failed: {res.stderr}")
        self.assertIn("pcrecomp Runtime Coverage Report", res.stdout)
        self.assertIn("Functions registered:", res.stdout)

    def test_17_render_path_and_first_frame_fl9_fl10(self):
        cmd = [str(self.exe_path), "--headless", "--frames", "5"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Render path test failed: {res.stderr}")

    def test_18_developer_recomp_tool_cli(self):
        py = sys.executable
        cli = self.repo_root / "scripts" / "tools" / "recomp_tool.py"
        res = subprocess.run([py, str(cli), "inspect", "0x005318A0"], cwd=str(self.repo_root), capture_output=True, text=True)
        self.assertEqual(res.returncode, 0, f"recomp_tool.py inspect failed: {res.stderr}")
        self.assertIn("Function Inspection: 0x005318A0", res.stdout)
        self.assertIn("Inferred Conv:    cdecl", res.stdout)

    def test_19_dxvk_native_initialization(self):
        cmd = [str(self.exe_path), "--frames", "5"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"DXVK-Native execution failed: {res.stderr}")
        self.assertIn("Loaded Direct3D 9 runtime from: thirdparty/dxvk-native/lib/libdxvk_d3d9.so", res.stdout)
        self.assertIn("Direct3D 9 guest-to-host COM thunking layer initialized", res.stdout)
        self.assertIn("Recomp runtime completed cleanly.", res.stdout)

    @legacy_only
    def test_20_frontend_textures_and_sprites(self):
        cmd = [str(self.exe_path), "--headless", "--test-menu"]
        res = self.run_cmd(cmd)
        self.assertEqual(res.returncode, 0, f"Frontend menu & texture test failed: {res.stderr}")
        self.assertIn("Validating authentic CMenuManager state machine (0x00574F30)", res.stdout)
        self.assertIn("Authentic CMenuManager state machine validated successfully", res.stdout)


if __name__ == "__main__":
    unittest.main()
