"""Guard against resurrecting automatic guest no-op bridges in the lift driver."""
import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch, Mock

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("lift_driver", ROOT / "scripts/analysis/run_lift.py")
driver = importlib.util.module_from_spec(spec)
spec.loader.exec_module(driver)


class StrictLiftTests(unittest.TestCase):
    def check_profile(self, adapter, expected):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            manifest = base / "target.toml"
            manifest.write_text('[target]\ntitle="Synthetic"\nactive_revision="' +
                                base.as_posix() + '"\ndefault_game_dir="' + base.as_posix() +
                                '"\nguest_executable="synthetic.exe"\nadapter="' + adapter + '"\n')
            emit = Mock()
            with patch.object(sys, "argv", ["run_lift.py", "--target-config", str(manifest),
                                           "--output", str(base / "output")]), \
                 patch.multiple(driver, load_revision_data=Mock(return_value=([], [])),
                                discover_direct_callees=Mock(return_value={0x401000: 16}),
                                generate_recomp_globals_cpp=Mock(), generate_recomp_funcs_h=Mock(),
                                generate_game_cpp=Mock(), generate_recomp_dispatch_cpp=emit):
                driver.main()
            self.assertEqual(emit.call_args.kwargs["auto_bridges"], expected)

    def test_lego_does_not_emit_discovered_return_stubs(self):
        self.check_profile("lego_batman", {})
