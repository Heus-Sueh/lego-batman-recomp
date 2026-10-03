import subprocess
import tempfile
import unittest
from pathlib import Path
from test_runtime import ACTIVE_TARGET


@unittest.skipUnless(ACTIVE_TARGET["adapter"] == "lego_batman", "LEGO Batman adapter contract")
class LegoAdapterTests(unittest.TestCase):
    root = Path(__file__).resolve().parent.parent
    binary = root / "out/build/linux-amd64-release" / ACTIVE_TARGET["output_name"]

    def run_adapter(self, *args):
        return subprocess.run([str(self.binary), *args], cwd=self.root,
                              capture_output=True, text=True, timeout=20)

    def test_legacy_modes_rejected_before_loading(self):
        for option in ("--test-menu", "--test-splash", "--test-raster", "--test-im3d", "--fps"):
            with self.subTest(option=option):
                result = self.run_adapter("--headless", option)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Unsupported option", result.stderr)
                self.assertNotIn("[GuestMemory]", result.stdout)

    def test_explicit_missing_directory_never_falls_back(self):
        with tempfile.TemporaryDirectory() as directory:
            result = self.run_adapter("--headless", "--game-dir", directory)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Missing executable", result.stderr)
            self.assertNotIn("[GuestMemory]", result.stdout)

    def test_same_size_invalid_pe_rejected(self):
        # Synthetic sparse file contains no proprietary bytes.
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / ACTIVE_TARGET["guest_executable"]
            with path.open("wb") as file:
                file.truncate(7557120)
            result = self.run_adapter("--headless", "--game-dir", directory)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("PE identity", result.stderr)
            self.assertNotIn("[GuestMemory]", result.stdout)

    def test_invalid_bound_rejected(self):
        for bound in ("0", "-1", "1junk"):
            self.assertNotEqual(self.run_adapter("--frames", bound).returncode, 0)
        self.assertNotEqual(self.run_adapter("--frames").returncode, 0)

    def test_bounded_entry_has_no_legacy_initialization(self):
        result = self.run_adapter("--headless", "--frames", "1", "--coverage")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("adapter: lego_batman", result.stdout)
        self.assertIn("Functions runtime reached:     27", result.stdout)
        self.assertIn("Total function dispatches:     30", result.stdout)
        self.assertIn("Unresolved indirect calls:     0", result.stdout)
        self.assertNotIn("[RenderWare]", result.stdout)
        self.assertNotIn("RsGlobal", result.stdout)

    def test_strict_frontier_reports_generated_missing_call(self):
        result = self.run_adapter("--headless", "--frames", "1", "--frontier", "--coverage")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ICALL: unresolved VA 0x00747462 from 0x0073D4C3", result.stderr)
        self.assertIn("Target VA:          0x00747462", result.stdout)
        self.assertIn("Unresolved indirect calls:     1", result.stdout)
