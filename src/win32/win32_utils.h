/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
 */
#ifndef WIN32_UTILS_H_
#define WIN32_UTILS_H_

#include <windows.h>
#include <winuser.h>
#include <strsafe.h>
#include <debugapi.h>
#include <errhandlingapi.h>

#define DebugPrint(...) OutputDebugStringA(__VA_ARGS__)

static void
PrintLastError(LPCWSTR FuncThatSetError)
{
    LPVOID MessageBuffer;
    LPTSTR DisplayBuffer;
    DWORD  ErrorCode = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                  ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&MessageBuffer, 0, NULL);

    DisplayBuffer = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)MessageBuffer) + lstrlen(FuncThatSetError) + 40)
                                                          * sizeof(TCHAR));

    StringCchPrintf(DisplayBuffer, LocalSize(DisplayBuffer) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"),
                    FuncThatSetError, ErrorCode, MessageBuffer);

    OutputDebugString(DisplayBuffer);

    LocalFree(MessageBuffer);
    LocalFree(DisplayBuffer);
}

#endif // WIN32_UTILS_H_
