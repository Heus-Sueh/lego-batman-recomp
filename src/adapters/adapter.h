#pragma once

namespace pcrecomp::adapter {
// Implemented by exactly one adapter selected at configure time.
// Owns application integration, diagnostics and lifecycle; returns a process code.
int run(int argc, char* argv[]);
}
