/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
 */

#include "isa.h"

ISA_LOG_REGISTER(Scn);

#include "consts.h"
#include "resources.h"
#include "scn_math.h"
#include "scn_intrinsics.h"
#include "scn.h"

#include "win32_utils.h"

isa_internal void
UpdateBg(u32_argb NewColor)
{
    Bg.Color = NewColor;
}

isa_internal void
UpdateBgColorWhenDragging(bg_landscape *Landscape, i64 x, i64 y)
{
    Landscape->x += x;
    Landscape->y += y;

    Landscape->Color.r += (u8)(x / 128);
    Landscape->Color.g += (u8)((x + y) / 128);
    Landscape->Color.b += (u8)(y / 128);
}

isa_internal scn_state *
InitScnState(scn_mem *Mem)
{
    scn_state *State = (scn_state *)Mem->Permanent;
    if(!Mem->Initialized)
    {
        State->Arena
            = IsaArenaCreate((u8 *)Mem->Permanent + sizeof(scn_state), Mem->PermanentMemSize - sizeof(scn_state));
        Mem->Initialized = true;
    }

    return State;
}

isa_internal u32_argb
ArgbFromu32(u32 Color)
{
    u32_argb Result;
    Result.U32 = Color;
    return Result;
}

isa_internal void
DrawRect(scn_offscreen_buffer Buffer, i64 X, i64 Y, i64 Width, i64 Height, u32_argb Color)
{
    // TODO(ingar): Bounds checking
    i64 Pitch = Buffer.w * Buffer.BytesPerPixel;
    u8 *Row   = ((u8 *)Buffer.Mem) + (Y * Pitch) + X;

    for(i64 y = Y; y < Y + Height; ++y)
    {
        u32 *Pixel = (u32 *)Row;
        for(i64 x = X; x < X + Width; ++x)
        {
            *Pixel++ = Color.U32;
        }

        Row += Pitch;
    }
}
// NOTE(ingar): Casey says that your code should not be split up in this way
// the code that updates state and then renders should be executed
// simultaneously so we might want to do that NOTE(ingar): This was also in the
// context of games. Sinuce we're a traditional app we might have different
// needs to handle events asynchronously

extern "C" RESPOND_TO_MOUSE(RespondToMouse)
{
    scn_state *ScnState = InitScnState(&Mem);
    IsaAssert(false, "Log message");

    // TODO(ingar): Convert to switch
    if(Event.Type == ScnMouseEvent_LDown)
    {
        MouseHistory.LClicked = true;
    }
    else if(Event.Type == ScnMouseEvent_RDown)
    {
        MouseHistory.RClicked = true;
    }

    if(Event.Type == ScnMouseEvent_LUp)
    {
        if(MouseHistory.Prev.Type == ScnMouseEvent_Move)
        {
            i64 PrevX = MouseHistory.PrevLClick.x;
            i64 PrevY = MouseHistory.PrevLClick.y;

            rect NewRectDim = { V2(0, 0), V2(0, 0) };

            if(PrevX < Event.x)
            {
                NewRectDim.min.x = (float)PrevX;
                NewRectDim.max.x = (float)Event.x;
            }
            else
            {
                NewRectDim.min.x = (float)Event.x;
                NewRectDim.max.x = (float)PrevX;
            }

            if(PrevY < Event.x)
            {
                NewRectDim.min.y = (float)PrevY;
                NewRectDim.max.y = (float)Event.y;
            }
            else
            {
                NewRectDim.min.y = (float)Event.y;
                NewRectDim.max.y = (float)PrevY;
            }

            // TODO(ingar): We need to push ui elements into memory which
            // UpdateBackBuffer then iterates over and draws
            // TODO(ingar): Since the functions are ran on timers, this
            // probably means that we need synchronization mechanisms so that
            // new elements are not pushed simultaneously with the drawing
            u32_argb NewRectColor;
            NewRectColor.U32 = GetRandu32();

            gui_rect *NewRect = IsaPushStructZero(&ScnState->Arena, gui_rect);
            NewRect->Dim      = NewRectDim;
            // PushStruct(ScnState->Arena, NewRect);
        }

        MouseHistory.LClicked = false;
    }

    if(Event.Type == ScnMouseEvent_RUp)
    {
        MouseHistory.RClicked = false;
    }

    MouseHistory.Prev = Event;
}

// NOTE(ingar): Man, this is overkill for this. Hoowee
extern "C" SEED_RAND_PCG(SeedRandPcg)
{
    SeedRandPcg_(Seed);
}

extern "C" UPDATE_BACK_BUFFER(UpdateBackBuffer)
{
    scn_state *ScnState = InitScnState(&Mem);

    /* Draw background */
    DrawRect(Buffer, 0, 0, Buffer.w, Buffer.h, Bg.Color);
}
