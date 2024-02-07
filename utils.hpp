#pragma once

#include <windows.h>
#include <winuser.h>
#include <strsafe.h>
#include <debugapi.h>
#include <errhandlingapi.h>
#include <stdint.h>
#include <assert.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

#define DebugPrint(...) OutputDebugStringA(__VA_ARGS__)

static void
PrintLastError(LPCWSTR FuncThatSetError)
{
    LPVOID MessageBuffer;
    LPTSTR DisplayBuffer;
    DWORD ErrorCode = GetLastError(); 
    
    FormatMessage(
                  FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  ErrorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &MessageBuffer,
                  0, NULL );
    
    DisplayBuffer = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, 
                                       (lstrlen((LPCTSTR)MessageBuffer) + lstrlen(FuncThatSetError) + 40) * sizeof(TCHAR));
    
    StringCchPrintf(DisplayBuffer, 
                    LocalSize(DisplayBuffer) / sizeof(TCHAR),
                    TEXT("%s failed with error %d: %s"),
                    FuncThatSetError, ErrorCode, MessageBuffer); 
    
    OutputDebugString(DisplayBuffer);
    
    LocalFree(MessageBuffer);
    LocalFree(DisplayBuffer);
}

#define internal static
#define local_persist static
#define global_variable static

#define Kilobytes(Value) ((Value) * 1024ULL)
#define Megabytes(Value) (Kilobytes(Value) * 1024ULL)
#define Gigabytes(Value) (Megabytes(Value) * 1024ULL)
#define Terabytes(Value) (Gigabytes(Value) * 1024ULL)


