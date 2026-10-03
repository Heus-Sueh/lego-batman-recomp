#include "win32_shims.h"
#include <cstring>

namespace pcrecomp::win32 {

int32_t shim_ov_open_callbacks(void* datasource, void* vf, const char* initial, int64_t ibytes, void* callbacks) {
    (void)datasource;
    (void)vf;
    (void)initial;
    (void)ibytes;
    (void)callbacks;
    return 0; // Success
}

int32_t shim_ov_clear(void* vf) {
    (void)vf;
    return 0; // Success
}

int64_t shim_ov_read(void* vf, char* buffer, int32_t length, int32_t bigendianp, int32_t word, int32_t sgned, int32_t* bitstream) {
    (void)vf;
    (void)bigendianp;
    (void)word;
    (void)sgned;
    (void)bitstream;
    if (buffer && length > 0) {
        std::memset(buffer, 0, length);
    }
    return 0; // EOF
}

} // namespace pcrecomp::win32
