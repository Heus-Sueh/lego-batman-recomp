#include <pcrecomp/memory.h>
#include <pcrecomp/dispatch.h>
#include <pcrecomp/imports.h>
#include <pcrecomp/callbacks.h>
#include <pcrecomp/modules.h>
#include <pcrecomp/win32_time.h>
/* Compiled as C, linked with the C++ implementation. */
int c_generated_call(PcRuntime* r, PcContext* ctx) {
    return pc_call_import(r,PC_TIME_GET_TICK_COUNT,ctx,0x120);
}
