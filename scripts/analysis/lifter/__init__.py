"""pcrecomp static recompilation lifter and code generator package."""

from .common import HEADER_COMMENT, write_if_changed, get_shared_cs
from .analysis import discover_stack_purge, discover_direct_callees
from .dispatch_gen import (
    generate_recomp_globals_cpp,
    generate_recomp_funcs_h,
    generate_recomp_dispatch_cpp,
)
from .game_lifter import generate_game_cpp

__all__ = [
    "HEADER_COMMENT",
    "write_if_changed",
    "get_shared_cs",
    "discover_stack_purge",
    "discover_direct_callees",
    "generate_recomp_globals_cpp",
    "generate_recomp_funcs_h",
    "generate_recomp_dispatch_cpp",
    "generate_game_cpp",
]
