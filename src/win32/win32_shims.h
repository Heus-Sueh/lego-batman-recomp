#pragma once

#include "win32_types.h"
#include <cstdint>
#include <cstddef>

namespace pcrecomp::win32 {

// KERNEL32 Timing
uint32_t shim_timeGetTime();
uint32_t shim_GetTickCount();
int32_t  shim_QueryPerformanceCounter(int64_t* lpPerformanceCount);
int32_t  shim_QueryPerformanceFrequency(int64_t* lpFrequency);

// KERNEL32 Process, Thread & Synchronization
void*    shim_CreateMutexA(void* lpMutexAttributes, int32_t bInitialOwner, const char* lpName);
int32_t  shim_ReleaseMutex(void* hMutex);
uint32_t shim_GetCurrentProcessId();
uint32_t shim_GetCurrentThreadId();
void     shim_OutputDebugStringA(const char* lpOutputString);
uint32_t shim_GetLastError();
void     shim_SetLastError(uint32_t dwErrCode);
void     shim_ExitProcess(uint32_t uExitCode);
void*    shim_GetProcAddress(void* hModule, const char* lpProcName);
uint32_t shim_TlsAlloc();
int32_t  shim_TlsSetValue(uint32_t dwTlsIndex, void* lpTlsValue);
void*    shim_TlsGetValue(uint32_t dwTlsIndex);
int32_t  shim_TlsFree(uint32_t dwTlsIndex);
int32_t  shim_InterlockedIncrement(int32_t* lpAddend);
int32_t  shim_InterlockedDecrement(int32_t* lpAddend);

// KERNEL32 Memory
void*    shim_VirtualAlloc(void* lpAddress, size_t dwSize, uint32_t flAllocationType, uint32_t flProtect);
int32_t  shim_VirtualProtect(void* lpAddress, size_t dwSize, uint32_t flNewProtect, uint32_t* lpflOldProtect);
int32_t  shim_VirtualFree(void* lpAddress, size_t dwSize, uint32_t dwFreeType);
void*    shim_HeapCreate(uint32_t flOptions, size_t dwInitialSize, size_t dwMaximumSize);
int32_t  shim_HeapDestroy(void* hHeap);
size_t   shim_HeapSize(void* hHeap, uint32_t dwFlags, const void* lpMem);

// KERNEL32 File I/O (routed through VirtualFileSystem)
void*    shim_CreateFileA(const char* lpFileName, uint32_t dwDesiredAccess, uint32_t dwShareMode,
                          void* lpSecurityAttributes, uint32_t dwCreationDisposition,
                          uint32_t dwFlagsAndAttributes, void* hTemplateFile);
int32_t  shim_ReadFile(void* hFile, void* lpBuffer, uint32_t nNumberOfBytesToRead,
                       uint32_t* lpNumberOfBytesRead, void* lpOverlapped);
int32_t  shim_WriteFile(void* hFile, const void* lpBuffer, uint32_t nNumberOfBytesToWrite,
                        uint32_t* lpNumberOfBytesWritten, void* lpOverlapped);
int32_t  shim_CloseHandle(void* hObject);
uint32_t shim_SetFilePointer(void* hFile, int32_t lDistanceToMove, int32_t* lpDistanceToMoveHigh, uint32_t dwMoveMethod);
uint32_t shim_GetFileSize(void* hFile, uint32_t* lpFileSizeHigh);
uint32_t shim_GetFileAttributesA(const char* lpFileName);
uint32_t shim_fopen_handle(const char* filename, const char* mode);

void*    shim_GetModuleHandleA(const char* lpModuleName);
const char* shim_GetCommandLineA(void);
void     shim_GetStartupInfoA(void* lpStartupInfo);
int32_t  shim_GetVersionExA(void* lpVersionInformation);
void*    shim_GetStdHandle(int32_t nStdHandle);
uint32_t shim_GetFileType(void* hFile);
uint32_t shim_SetHandleCount(uint32_t uNumber);
void*    shim_GetEnvironmentStringsW(void);
int32_t  shim_FreeEnvironmentStringsW(void* penv);
int32_t  shim_WideCharToMultiByte(uint32_t CodePage, uint32_t dwFlags, const uint16_t* lpWideCharStr,
                                 int32_t cchWideChar, char* lpMultiByteStr, int32_t cbMultiByte,
                                 const char* lpDefaultChar, int32_t* lpUsedDefaultChar);
uint32_t shim_GetACP(void);
uint32_t shim_GetOEMCP(void);
int32_t  shim_IsValidCodePage(uint32_t CodePage);
int32_t  shim_GetCPInfo(uint32_t CodePage, void* lpCPInfo);
void*    shim_CreateEventA(void* lpEventAttributes, int32_t bManualReset, int32_t bInitialState, const char* lpName);
void     shim_Sleep(uint32_t dwMilliseconds);
void     shim_PostQuitMessage(int32_t nExitCode);
void*    shim_SetUnhandledExceptionFilter(void* lpTopLevelExceptionFilter);

// USER32 Window & Message Operations
void*    shim_CreateWindowExA(uint32_t dwExStyle, const char* lpClassName, const char* lpWindowName,
                              uint32_t dwStyle, int32_t X, int32_t Y, int32_t nWidth, int32_t nHeight,
                              void* hWndParent, void* hMenu, void* hInstance, void* lpParam);
int32_t  shim_ShowWindow(void* hWnd, int32_t nCmdShow);
int32_t  shim_DestroyWindow(void* hWnd);
int32_t  shim_AdjustWindowRectEx(RECT* lpRect, uint32_t dwStyle, int32_t bMenu, uint32_t dwExStyle);
int32_t  shim_AdjustWindowRect(RECT* lpRect, uint32_t dwStyle, int32_t bMenu);
int32_t  shim_SetWindowTextA(void* hWnd, const char* lpString);
int32_t  shim_PeekMessageA(void* lpMsg, void* hWnd, uint32_t wMsgFilterMin, uint32_t wMsgFilterMax, uint32_t wRemoveMsg);
int32_t  shim_DefWindowProcA(void* hWnd, uint32_t Msg, uint64_t wParam, int64_t lParam);
int32_t  shim_ShowCursor(int32_t bShow);
void*    shim_LoadIconA(void* hInstance, const char* lpIconName);
void*    shim_LoadCursorA(void* hInstance, const char* lpCursorName);
uint16_t shim_RegisterClassA(const void* lpWndClass);
uint16_t shim_RegisterClassExA(const void* lpwcx);
int32_t  shim_SendMessageA(void* hWnd, uint32_t Msg, uint64_t wParam, int64_t lParam);
void*    shim_FindWindowA(const char* lpClassName, const char* lpWindowName);
int32_t  shim_SetForegroundWindow(void* hWnd);
int32_t  shim_GetWindowPlacement(void* hWnd, void* lpwndpl);
int32_t  shim_SystemParametersInfoA(uint32_t uiAction, uint32_t uiParam, void* pvParam, uint32_t fWinIni);
uint32_t shim_SetErrorMode(uint32_t uMode);
int32_t  shim_SetWindowLongA(void* hWnd, int32_t nIndex, int32_t dwNewLong);
int32_t  shim_UpdateWindow(void* hWnd);
int32_t  shim_SetWindowPos(void* hWnd, void* hWndInsertAfter, int32_t X, int32_t Y, int32_t cx, int32_t cy, uint32_t uFlags);

// vorbisfile audio stubs
int32_t  shim_ov_open_callbacks(void* datasource, void* vf, const char* initial, int64_t ibytes, void* callbacks);
int32_t  shim_ov_clear(void* vf);
int64_t  shim_ov_read(void* vf, char* buffer, int32_t length, int32_t bigendianp, int32_t word, int32_t sgned, int32_t* bitstream);

void initialize_win32_subsystems();

} // namespace pcrecomp::win32
