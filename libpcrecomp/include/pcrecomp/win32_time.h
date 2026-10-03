#ifndef PCRECOMP_WIN32_TIME_H
#define PCRECOMP_WIN32_TIME_H
#include "imports.h"
#ifdef __cplusplus
extern "C" {
#endif
enum PcTimeImport {
    PC_TIME_GET_TICK_COUNT = 1, PC_TIME_GET_TIME = 2,
    PC_TIME_QUERY_COUNTER = 3, PC_TIME_QUERY_FREQUENCY = 4
};
/* Optional adapter target: x86 stdcall arguments at esp+4, includes ret pop. */
PcStatus pc_register_time_imports(PcRuntime*);
#ifdef __cplusplus
}
#endif
#endif
