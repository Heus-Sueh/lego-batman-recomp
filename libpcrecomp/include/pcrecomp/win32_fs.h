#ifndef PCRECOMP_WIN32_FS_H
#define PCRECOMP_WIN32_FS_H
#include "imports.h"
#ifdef __cplusplus
extern "C" {
#endif
enum PcFsImport {
    PC_FS_CREATE_FILE_A = 10,
    PC_FS_READ_FILE = 11,
    PC_FS_WRITE_FILE = 12,
    PC_FS_CLOSE_HANDLE = 13,
    PC_FS_SET_FILE_POINTER = 14,
    PC_FS_GET_FILE_SIZE = 15
};
/* Optional adapter target: x86 stdcall arguments on guest stack, includes ret pop. */
PcStatus pc_register_fs_imports(PcRuntime*);
#ifdef __cplusplus
}
#endif
#endif
