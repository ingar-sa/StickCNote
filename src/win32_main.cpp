/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
*/


#include "isa.hpp"

#include "utils.hpp"
#include "app.hpp"

#include "consts.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <winuser.h>
#include <tchar.h>
#include <shellapi.h>
#include <libloaderapi.h>


internal struct app_data
{
    app_mem Mem;
    offscreen_buffer OffscreenBuffer;

    TCHAR DllName[MAX_PATH]; // TODO(ingar): MAX_PATH is deprecated 
    TCHAR TempDllName[MAX_PATH];

    HMODULE Dll;
    FILETIME LastWriteTime;

    update_back_buffer *UpdateBackBuffer;
    respond_to_mouse_click *RespondToMouseClick;
    respond_to_mouse_hover *RespondToMouseHover;  

    bool CodeLoaded;

} AppData;

internal struct win32_data
{ 
    NOTIFYICONDATA NotifyIconData = {0};
    HICON AppIcon = {0};

} Win32Data;

enum :  UINT 
{
    WM_TRAY_ICON = (WM_USER + 1),
    ID_TRAY_EXIT,
    ID_TRAY_SHOW,
    ID_TRAY_APP_ICON,

    WIN32_COMMANDS_FINAL,
};

constexpr u64 NUM_COMMANDS = WIN32_COMMANDS_FINAL - WM_TRAY_ICON;

internal struct window_buffer
{
    BITMAPINFO DIBInfo;

    LONG Width;
    LONG Height;

    LONG BytesPerPixel = 4;

    void *Mem;

} WindowBuffer;

struct win32_window_dims
{
    LONG Width, Height;
};


internal u64 
StringLengthTchar(const TCHAR *String)
{
    u64 Count = 0;
    while(*String++)
    {
        ++Count;
    }
    return Count;
}

internal i64
FindLastOfTChar(const TCHAR *StringToSearch, TCHAR Char)
{
    i64 LastIndex = -1;  // Use an invalid index to indicate not found.
    for(u64 Index = 0; StringToSearch[Index]; ++Index)
    {
        if(StringToSearch[Index] == Char) LastIndex = Index;
    }

    return LastIndex;
}

internal void
CatStringsTchar(u64 SourceACount, const TCHAR *SourceA,
                u64 SourceBCount, const TCHAR *SourceB,
                u64 DestCount,          TCHAR *Dest)
{
    IsaAssert(SourceACount + SourceBCount < DestCount);

    for(u64 Index = 0; Index < SourceACount; ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for(u64 Index = 0; Index < SourceBCount; ++Index)
    {
        *Dest++ = *SourceB++;
    }
    
    *Dest++ = 0;
}

internal bool
AppendToEXEFilePathTchar(const TCHAR *Filename, TCHAR *Out, size_t OutLen)
{
    HMODULE hModule = GetModuleHandle(NULL);
    if(!hModule)
    {
        // TODO(ingar): Logging
        return false;
    }

    TCHAR CurrentPath[MAX_PATH]; // TODO(ingar): MAX_PATH is no longer correct, so we have to make this more robust
    DWORD Len = GetModuleFileName(hModule, CurrentPath, MAX_PATH);
    if(!Len)
    {
        //TODO(ingar): Logging
        return false;
    }

    size_t PathSize = FindLastOfTChar(CurrentPath, '\\') + 1;
    CatStringsTchar(PathSize, CurrentPath, StringLengthTchar(Filename), Filename, OutLen, Out);

    return true;
}


inline FILETIME
Win32GetLastWriteTime(const TCHAR *Filename)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return(LastWriteTime);
}

internal void
Win32UnloadAppData(void)
{
    if(AppData.Dll)
    {
        FreeLibrary(AppData.Dll);
        AppData.Dll= 0;
    }

    AppData.CodeLoaded = false;
    AppData.UpdateBackBuffer = NULL;
}

internal bool
Win32LoadAppData(void) // TODO(ingar): Pass file path as argument?
{
    Win32UnloadAppData();

    bool CopySucceeded = CopyFile(AppData.DllName, AppData.TempDllName, FALSE);
    if(!CopySucceeded)
    {
        PrintLastError(TEXT("CopyFile"));
        return false;
    }

    HMODULE Dll = LoadLibraryExW(AppData.TempDllName, 0, 0);
    if(!Dll)
    {
        PrintLastError(TEXT("LoadLibraryExW)"));
        return false;// {0};
    }

    update_back_buffer  *UpdateBackbuffer = (update_back_buffer *)GetProcAddress(Dll, "UpdateBackBuffer");
    respond_to_mouse_click *RespondToMouseClick = (respond_to_mouse_click *)GetProcAddress(Dll, "RespondToMouseClick");
    respond_to_mouse_hover *RespondToMouseHover = (respond_to_mouse_hover *)GetProcAddress(Dll, "RespondToMouseHover");
    if(!UpdateBackbuffer)
    {
        PrintLastError(TEXT("GetProcAddress"));
        return false; //{0};
    }

    AppData.Dll = Dll;
    AppData.UpdateBackBuffer = UpdateBackbuffer;
    AppData.RespondToMouseClick = RespondToMouseClick;
    AppData.RespondToMouseHover = RespondToMouseHover;
    AppData.CodeLoaded = true;
    
    return true;
}

VOID CALLBACK
Win32UpdateAppDataTimer(HWND Window, UINT Message, UINT_PTR TimerId, DWORD Time)
{
    FILETIME LastFileTime = Win32GetLastWriteTime(AppData.DllName);
    
    if(CompareFileTime(&LastFileTime, &AppData.LastWriteTime))
    {
        DebugPrint("Write time is newere!\n");

        bool Success = Win32LoadAppData(); 
        if(Success)
        {
            DebugPrint("Successfully updated Oms code in timer\n");    
            AppData.LastWriteTime = LastFileTime;
            
        }
    }
    else
    {
//        OmsDebugPrint("Last write time not greater than previous write time\n");
    }
}

internal win32_window_dims 
Win32GetWindowDimensions(HWND Window)
{
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);

    LONG Width = ClientRect.right - ClientRect.left;
    LONG Height = ClientRect.bottom - ClientRect.top;

    return {Width, Height};
}


internal void
Win32AddTrayIcon(HWND Window)
{ 
    NOTIFYICONDATA IconData = Win32Data.NotifyIconData;

    memset(&IconData, 0, sizeof(NOTIFYICONDATA));
    
    IconData.cbSize = sizeof(NOTIFYICONDATA);
    IconData.hWnd = Window;
    IconData.uID = ID_TRAY_APP_ICON; 
    IconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    IconData.uCallbackMessage = WM_TRAY_ICON; 
    IconData.hIcon = Win32Data.AppIcon; 
    lstrcpy(IconData.szTip, TEXT("StickCNote")); 

    Shell_NotifyIcon(NIM_ADD, &IconData); 
}

internal void
Win32RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &Win32Data.NotifyIconData); // Remove the icon from the system tray
}

internal void
Win32ResizeDibSection(LONG Width, LONG Height)
{
    WindowBuffer.Width = Width;
    WindowBuffer.Height = Height;

    WindowBuffer.DIBInfo.bmiHeader.biSize = sizeof(WindowBuffer.DIBInfo.bmiHeader);
    WindowBuffer.DIBInfo.bmiHeader.biWidth = WindowBuffer.Width;
    WindowBuffer.DIBInfo.bmiHeader.biHeight = -WindowBuffer.Height; // - to start at the top of the screen
                                                          //
    WindowBuffer.DIBInfo.bmiHeader.biPlanes = 1;
    WindowBuffer.DIBInfo.bmiHeader.biBitCount = 32;
    WindowBuffer.DIBInfo.bmiHeader.biCompression = BI_RGB;

    WindowBuffer.Mem = AppData.Mem.Work; 
}

internal void
Win32UpdateWindow(HDC DeviceContext,  LONG Width, LONG Height)
{
    // NOTE(ingar): A back buffer is a subset of offscreen buffers that is specifically
    // meant to hold the next frame to be displayed, which is appropriate in this circumstance
    offscreen_buffer BackBuffer = {};

    BackBuffer.w = WindowBuffer.Width;
    BackBuffer.h = WindowBuffer.Height;

    BackBuffer.Mem = WindowBuffer.Mem;
    BackBuffer.BytesPerPixel = WindowBuffer.BytesPerPixel;

    AppData.UpdateBackBuffer(BackBuffer);

    StretchDIBits(DeviceContext,
                  0, 0, Width, Height,
                  0, 0, WindowBuffer.Width, WindowBuffer.Height,
                  WindowBuffer.Mem, &WindowBuffer.DIBInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

VOID CALLBACK
Win32RedrawWindowTimer(HWND Window, UINT Message, UINT_PTR TimerId, DWORD Time)
{
//    OmsDebugPrint("Entered redraw timer\n");
    HDC DeviceContext = GetDC(Window);
    win32_window_dims WindowDimensions = Win32GetWindowDimensions(Window);
    Win32UpdateWindow(DeviceContext, WindowDimensions.Width, WindowDimensions.Height);
    ReleaseDC(Window, DeviceContext);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT SystemMessage,
                        WPARAM WParams,
                        LPARAM LParams)
{
    LRESULT CallbackResult = 0;

    switch(SystemMessage)
    {
        case WM_TRAY_ICON:
        {
            if (LParams == WM_RBUTTONDOWN)
            {
                POINT Point;
                GetCursorPos(&Point);

                HMENU Menu = CreatePopupMenu();
                InsertMenu(Menu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));
                InsertMenu(Menu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_SHOW, TEXT("Show"));
                
                TrackPopupMenu(Menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, Point.x, Point.y, 0, Window, NULL);
                DestroyMenu(Menu);
            }
            else if(LParams == WM_LBUTTONDOWN)
            {
                ShowWindow(Window, SW_SHOW);
            }
        }
        break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            POINT CursorPos = { LOWORD(LParams), HIWORD(LParams) };
            AppData.RespondToMouseClick(SystemMessage, CursorPos);
        }
        break;

        case WM_MOUSEMOVE:
        {
            POINT CursorPos = { LOWORD(LParams), HIWORD(LParams) };
            AppData.RespondToMouseHover(CursorPos);
        }
        break;
        case WM_COMMAND:
        {
            switch(LOWORD(WParams))
            {
                case ID_TRAY_EXIT:
                {
                    PostQuitMessage(0);
                }
                break;

                case ID_TRAY_SHOW:
                {
                    ShowWindow(Window, SW_SHOW);
                }
                break;
               
            }
        }
        break;

        case WM_SYSCOMMAND:
        {
            if(WParams == SC_MINIMIZE)
            {
                Win32AddTrayIcon(Window);
                ShowWindow(Window, SW_HIDE);
                return 0;
            }

            return DefWindowProc(Window, SystemMessage, WParams, LParams);
        }
        break;

        case WM_CLOSE:
        {
            /*
            Win32AddTrayIcon(Window);
            ShowWindow(Window, SW_HIDE);
            */
            PostQuitMessage(0);
        }
        break;        

        case WM_DESTROY:
        {
            // TODO(ingar): Handle this as an error - recreate window?
            PostQuitMessage(0);
        }
        break;
        
        case WM_ACTIVATEAPP:
        {
        }
        break;

        case WM_SIZE:
        {
            win32_window_dims Dimensions = Win32GetWindowDimensions(Window);
            Win32ResizeDibSection(Dimensions.Width, Dimensions.Height);
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC PaintContext = BeginPaint(Window, &Paint);
            
            win32_window_dims Dimensions = Win32GetWindowDimensions(Window);
            Win32UpdateWindow(PaintContext, Dimensions.Width, Dimensions.Height);
            EndPaint(Window, &Paint);
        }
        break;

        case WM_HOTKEY:
        case WM_KEYDOWN:
        {
            //DebugPrint("A key was pressed down!\n");

        }
        break;

        case WM_KEYUP:

        default:
        {
            CallbackResult = DefWindowProc(Window, SystemMessage, WParams, LParams);
        }
        break;
    }
    
    return CallbackResult;
}

int CALLBACK
WinMain(HINSTANCE Instance, 
        HINSTANCE PrevInstance,
        LPSTR CommandLineString, 
        int WindowShowMode)
{
    
    WNDCLASSEX WindowClass = {};
    
    WindowClass.cbSize = sizeof(WNDCLASSEX);
    WindowClass.style  = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon = ; // TODO(Ingar): We need to fill this in when we have an icon!
    WindowClass.lpszClassName = TEXT("StickCNoteWindowClass");
    
    if(!RegisterClassEx(&WindowClass))
    {
        PrintLastError(TEXT("RegisterClassEx"));
        return FALSE;
    }

    HANDLE ProgramInstanceMutex = CreateMutex(NULL, FALSE, TEXT("StickCNoteProgramMutex"));
    if(GetLastError() == ERROR_ALREADY_EXISTS)
    {
        DebugPrint("An instance of the application already exists."
                   "This application only supports one instance!\n");
        return FALSE;
    }

    HWND Window = {}; 
    Window = CreateWindowEx(0,
                            WindowClass.lpszClassName,
                            TEXT("StickCNote"),
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Comment out WS_VISIBLE to start in minimized mode 
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            0,0,
                            Instance,
                            0);
    
    if(!Window) 
    {
        PrintLastError(TEXT("CreateWindowEx"));
        return FALSE; 
    }

    Win32Data.AppIcon = (HICON)LoadImage(NULL,
                      TEXT("W:/StickCNote/resources/icon_default.ico"),
                      IMAGE_ICON,
                      0, 0,
                      LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if(!Win32Data.AppIcon)
    {
        PrintLastError(TEXT("LoadImage"));
        return FALSE;
    }

    SendMessage(Window, WM_SETICON, ICON_BIG, (LPARAM)Win32Data.AppIcon);

    Win32AddTrayIcon(Window);

    AppendToEXEFilePathTchar(APP_DLL_NAME_TEXT, AppData.DllName, MAX_PATH);
    AppendToEXEFilePathTchar(APP_DLL_TEMP_NAME_TEXT, AppData.TempDllName, MAX_PATH);

    bool Succeded = Win32LoadAppData();
    if(!Succeded)
    {
        return FALSE;
    }
 
#ifndef NAPP_DEBUG 
    LPVOID BaseAddressPermanentMem = (LPVOID)TeraBytes(1);
    LPVOID BaseAddressWorkMem = (LPVOID)TeraBytes(2);
#else
    LPVOID BaseAddressPermanentMem = 0;
    LPVOID BaseAddressWorkMem = 0;
#endif

    AppData.Mem.PermanentMemSize = MegaBytes(64);
    AppData.Mem.Permanent = VirtualAlloc(BaseAddressPermanentMem, AppData.Mem.PermanentMemSize, 
                                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    AppData.Mem.WorkMemSize = MegaBytes(128);
    AppData.Mem.Work = VirtualAlloc(BaseAddressWorkMem, AppData.Mem.WorkMemSize, 
                                        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if(!(AppData.Mem.Permanent && AppData.Mem.Work))
    {
        PrintLastError(TEXT("VirtualAlloc"));
        return FALSE;
    }

    AppData.Mem.Initialized = true;

    
    UINT_PTR AppDataTimerId = SetTimer(Window, 1, 50, &Win32UpdateAppDataTimer);
    if(!AppDataTimerId)
    {
        PrintLastError(TEXT("SetTimer"));
        return FALSE;
    }
    
    UINT_PTR RedrawWindowTimer = SetTimer(Window, 2, 50, &Win32RedrawWindowTimer);
    if(!RedrawWindowTimer)
    {
        PrintLastError(TEXT("SetTimer"));
        return FALSE;
    }


    //NOTE(Ingar): We need to call this here because the WM_SIZE message is posted before the above memory allocation
    //             which meaans GlobalBackbuffer's memory's address is 0
    win32_window_dims WindowDimensions = Win32GetWindowDimensions(Window);
    Win32ResizeDibSection(WindowDimensions.Width, WindowDimensions.Height);
   
    MSG Message = {};
    BOOL MessageRet = 1;
    
    while((MessageRet = GetMessage(&Message, NULL, 0, 0)) != 0)
    {
        if(MessageRet == -1)
        {
            PrintLastError(TEXT("GetMessage"));
            return (int)Message.wParam;
        }
        else
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        
    }

    return (int)Message.wParam;
}

