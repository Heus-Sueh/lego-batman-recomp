# DXVK-native integration dependency

This directory contains compatibility headers and expects a locally supplied
DXVK-native D3D9 build in `lib/libdxvk_d3d9.so` (including its symlink targets).
The project does not build or download that dependency automatically.

Precompiled binaries previously committed here have been removed from the index
because their exact source revision/build provenance was not recorded. Existing
local copies can still be used for development; historical commits are unchanged.
Do not recommit them without reproducible provenance and a redistribution review.

Upstream: https://github.com/doitsujin/dxvk (native builds).
Upstream license: https://github.com/doitsujin/dxvk/blob/master/LICENSE.
The upstream zlib notice is retained in LICENSE. Headers also carry individual
notices; preserve these and COPYING.MinGW-w64.txt. This notice does not establish
the origin of any locally installed binary.

The game build currently requires this dependency even for headless startup.
`just test-runtime` does not. Source-built dependencies should record their commit,
compiler, build options and checksum before being proposed for redistribution.
