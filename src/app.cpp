/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
*/


#include "isa.hpp"
#include "app.hpp"

union u32_argb
{
    struct
    {
        u8 B, G, R, A;
    };
     
    u8 BGRA[4];
    u32 U32;
};

static u32_argb BgColor = {};

// void name(UINT WmCommand, POINT CursorPos)
extern "C" RESPOND_TO_MOUSE_CLICK(RespondToMouseClick)
{
    BgColor.R = (u8)CursorPos.y;
    BgColor.G = (u8)CursorPos.x;
    BgColor.B = (u8)CursorPos.x - (u8)CursorPos.y;
}

extern "C" RESPOND_TO_MOUSE_HOVER(RespondToMouseHover)
{
    BgColor.R = (u8)CursorPos.y;
    //BgColor.G = (u8)CursorPos.x + (u8)CursorPos.y;
    //BgColor.B = (u8)CursorPos.x - (u8)CursorPos.y;
}

// void name(offscreen_buffer Buffer)
extern "C" UPDATE_BACK_BUFFER(UpdateBackBuffer)
{
    i64 Pitch = Buffer.w * Buffer.BytesPerPixel;
    u8 *Row = (u8 *)Buffer.Mem;

    for(i64 y = 0; y < Buffer.h; ++y)
    {
        u32 *Pixel = (u32 *)Row;
        for(i32 x = 0; x < Buffer.w; ++x) {
            *Pixel++ = BgColor.U32;
        }

        Row += Pitch;
    }
} 

