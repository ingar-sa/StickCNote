/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
*/

//#include "isa.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <windows.h>
#include <winuser.h>
#include <tchar.h>
#include <shellapi.h>
#include <libloaderapi.h>

#include "utils.hpp"

internal constexpr i64 WM_TRAYICON = (WM_USER + 1);
internal constexpr WORD ID_TRAY_EXIT = 2222;
internal constexpr WORD ID_TRAY_APP_ICON = 2223;

internal NOTIFYICONDATA NotifyIconData = {0};

static struct app_mem
{
    bool Initialized;

    void *Permanent;
    u64   PermanentMemSize;

    void *Work;
    u64   WorkMemSize;
    
} AppMem;

internal struct window_buffer
{
    BITMAPINFO DIBInfo;

    i64 Width;
    i64 Height;

    i64 BytesPerPixel = 4;

    void *Mem;
} WindowBuffer;

struct win32_window_dims
{
    i64 Width, Height;
};


internal win32_window_dims 
Win32GetWindowDimensions(HWND Window)
{
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    i64 Width = ClientRect.right - ClientRect.left;
    i64 Height = ClientRect.bottom - ClientRect.top;

    return {Width, Height};
}


internal void
Win32AddTrayIcon(HWND hWnd)
{
    memset(&NotifyIconData, 0, sizeof(NOTIFYICONDATA));
    
    NotifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    NotifyIconData.hWnd = hWnd;
    NotifyIconData.uID = ID_TRAY_APP_ICON; // Identifier of the icon
    NotifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    NotifyIconData.uCallbackMessage = WM_TRAYICON; // Custom message to process tray icon events
    NotifyIconData.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Load a standard icon for the tray
    lstrcpy(NotifyIconData.szTip, TEXT("Ughhhhh... sticky...")); // Tooltip for the tray icon

    Shell_NotifyIcon(NIM_ADD, &NotifyIconData); // Add the icon to the system tray
}

internal void
Win32RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &NotifyIconData); // Remove the icon from the system tray
}

internal void
Win32ResizeDibSection(app_mem *Mem, window_buffer *Buffer, i64 Width, i64 Height)
{
    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->DIBInfo.bmiHeader.biSize = sizeof(Buffer->DIBInfo.bmiHeader);
    Buffer->DIBInfo.bmiHeader.biWidth = Buffer->Width;
    Buffer->DIBInfo.bmiHeader.biHeight = -Buffer->Height; // - to start at the top of the screen
    Buffer->DIBInfo.bmiHeader.biPlanes = 1;
    Buffer->DIBInfo.bmiHeader.biBitCount = 32;
    Buffer->DIBInfo.bmiHeader.biCompression = BI_RGB;

    Buffer->Mem = Mem->Work; 
}

internal void
Win32UpdateWindow(HDC DeviceContext, window_buffer *Buffer, i64 Width, i64 Height)
{
    StretchDIBits(DeviceContext,
            0, 0, Width, Height,
            0, 0, Buffer->Width, Buffer->Height,
            Buffer->Mem, &Buffer->DIBInfo,
            DIB_RGB_COLORS, SRCCOPY);
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
        case WM_TRAYICON:
        {
            if (LParams == WM_RBUTTONDOWN)
            {
                POINT Point;
                GetCursorPos(&Point);

                HMENU Menu = CreatePopupMenu();
                InsertMenu(Menu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));

                SetForegroundWindow(Window);
                TrackPopupMenu(Menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, Point.x, Point.y, 0, Window, NULL);
                DestroyMenu(Menu);
            }
        } break;

        case WM_COMMAND:
        {
            if (LOWORD(WParams) == ID_TRAY_EXIT)
            {
                Shell_NotifyIcon(NIM_DELETE, &NotifyIconData); // Remove tray icon
                PostQuitMessage(0);
            }
        } break;
        case WM_CLOSE:
        {
            // TODO(ingar): Handle this with a confirmation box?
            //              Correct disconnection from the cloud back-end might be necessary.
            //OmsOutputDebugString(TEXT("Windows has sent a WM_CLOSE message through the callback function\n"));
            /*
            if(MessageBox(Window, TEXT("Really quit?"), TEXT("My application"), MB_OKCANCEL) == IDOK)
            {
                DestroyWindow(Window);
            }
            */
            PostQuitMessage(0);
        } break;
        
        case WM_DESTROY:
        {
            // TODO(ingar): Handle this as an error - recreate window?
            PostQuitMessage(0);
        } break;
        
        case WM_ACTIVATEAPP:
        {
        } break;

            case WM_SIZE:
        {
            win32_window_dims Dimensions = Win32GetWindowDimensions(Window);
            Win32ResizeDibSection(&AppMem, &WindowBuffer, Dimensions.Width, Dimensions.Height);
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC PaintContext = BeginPaint(Window, &Paint);
            
            win32_window_dims Dimensions = Win32GetWindowDimensions(Window);
            Win32UpdateWindow(PaintContext, &WindowBuffer, Dimensions.Width, Dimensions.Height);
            EndPaint(Window, &Paint);
        } break;

        case WM_HOTKEY:
        case WM_KEYDOWN:
        {
            DebugPrint("A key was pressed down!\n");

        } break;
        case WM_KEYUP:
        default:
        {
            CallbackResult = DefWindowProc(Window, SystemMessage, WParams, LParams);
        } break;
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
    WindowClass.lpszClassName = TEXT("stickCnoteWindowClass");
    
    if(!RegisterClassEx(&WindowClass))
    {
        PrintLastError(TEXT("RegisterClassEx"));
        return FALSE;
    }

    HANDLE ProgramInstanceMutex = CreateMutex(NULL, FALSE, TEXT("stickCnoteProgramMutex"));
    if(GetLastError() == ERROR_ALREADY_EXISTS)
    {
        DebugPrint("An instance of the application already exists."
                   "This application only supports one instance!\n");
        return FALSE;
    }
    
    HWND Window = {}; 
    Window = CreateWindowEx(0,
                            WindowClass.lpszClassName,
                            TEXT("stickCnote"),
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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

#ifndef NOms_DEBUG
    LPVOID BaseAddressPermanentMem = (LPVOID)Terabytes(1);
    LPVOID BaseAddressWorkMem = (LPVOID)Terabytes(2);
#else
    LPVOID BaseAddressPermanentMem = 0;
    LPVOID BaseAddressWorkMem = 0;
#endif

    AppMem.PermanentMemSize = Megabytes(64);
    AppMem.Permanent = VirtualAlloc(BaseAddressPermanentMem, AppMem.PermanentMemSize, 
                                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    AppMem.WorkMemSize = Megabytes(128);
    AppMem.Work = VirtualAlloc(BaseAddressWorkMem, AppMem.WorkMemSize, 
                                        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if(!(AppMem.Permanent && AppMem.Work))
    {
        PrintLastError(TEXT("VirtualAlloc"));
        return FALSE;
    }

    AppMem.Initialized = true;

    //NOTE(Ingar): We need to call this here because the WM_SIZE message is posted before the above memory allocation
    //             which meaans GlobalBackbuffer's memory's address is 0
    win32_window_dims WindowDimensions = Win32GetWindowDimensions(Window);
    Win32ResizeDibSection(&AppMem, &WindowBuffer, WindowDimensions.Width, WindowDimensions.Height);
   
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

