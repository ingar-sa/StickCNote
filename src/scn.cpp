/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
*/


#include "isa.hpp"
#include "scn.hpp"


struct bg_landscape
{
    u32_argb Color;
    i64 x, y;
};

static bg_landscape Bg = {};

internal void 
UpdateBgColorWhenDragging(bg_landscape *Landscape, i64 x, i64 y)
{
   Landscape->x += x; 
   Landscape->y += y; 

   Landscape->Color.R += (u8)(x / 128);
   Landscape->Color.G += (u8)((x + y) / 128);
   Landscape->Color.B += (u8)(y / 128);
}



 // NOTE(ingar): Is it better to have everything in one function, or is there
 // merit to splitting it up in order to have a shorter code path (maybe) for
 // each action?
extern "C" RESPOND_TO_MOUSE(RespondToMouse)
{
    static bool LeftClicked = false;
    static bool RightClicked = false;

    if(Event == MOUSE_LDOWN) LeftClicked = true; 
    if(Event == MOUSE_RDOWN) RightClicked = true; 
    if(Event == MOUSE_LUP) LeftClicked = false; 
    if(Event == MOUSE_RUP) RightClicked = false; 

    if(Event == MOUSE_MOVE && LeftClicked)
    {
        UpdateBgColorWhenDragging(&Bg, x, y);
    }
}
/*
extern "C" RESPOND_TO_MOUSE_CLICK(RespondToMouseClick)
{
    BgColor.R = (u8)y;
    BgColor.G = (u8)x;
    BgColor.B = (u8)x - (u8)y;
}

extern "C" RESPOND_TO_MOUSE_HOVER(RespondToMouseHover)
{
    BgColor.R = (u8)y;
    BgColor.G = (u8)x + (u8)y;
    BgColor.B = (u8)x - (u8)y;
}
*/

extern "C" UPDATE_BACK_BUFFER(UpdateBackBuffer)
{
    i64 Pitch = Buffer.w * Buffer.BytesPerPixel;
    u8 *Row = (u8 *)Buffer.Mem;

    for(i64 y = 0; y < Buffer.h; ++y)
    {
        u32 *Pixel = (u32 *)Row;
        for(i32 x = 0; x < Buffer.w; ++x) {
            *Pixel++ = Bg.Color.U32;
        }

        Row += Pitch;
    }
}
