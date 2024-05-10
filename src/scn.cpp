/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
 */

#include "isa.h"

ISA_LOG_REGISTER(Scn);

#include "win32/resources.h"
#include "win32/win32_utils.h"

#include "consts.h"
#include "scn_math.h"
#include "scn_intrinsics.h"
#include "scn.h"

// TODO(ingar): All of the static structs should proabably be part of
// the permanent memory so that they can be stored on disk
isa_global struct mouse_history
{
    bool LClicked = false;
    bool RClicked = false;

    scn_mouse_event Prev;
    scn_mouse_event PrevLClick;
    scn_mouse_event PrevRClick;

    v2 PrevLClickPos;

} MouseHistory;

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

        const u32 MAX_NOTES = 1024;

        note_collection *NoteCollection = IsaPushStructZero(&State->Arena, note_collection);
        isa_arena       *NoteArena      = IsaPushStructZero(&State->Arena, isa_arena);
        IsaArenaCreate(NoteArena, IsaPushArray(&State->Arena, note, MAX_NOTES), MAX_NOTES * sizeof(note));
        const u64        MAX_NOTES      = 1024;
        note_collection *NoteCollection = IsaPushStructZero(&State->PermArena, note_collection);
        NoteCollection->MaxCount        = MAX_NOTES;
        NoteCollection->N               = IsaPushArray(&State->PermArena, note, MAX_NOTES);
        State->Notes                    = NoteCollection;

        NoteCollection->MaxCount = MAX_NOTES;
        NoteCollection->Arena    = NoteArena;
        State->Notes             = NoteCollection;

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

void
FillNote(note *Note, rect Rect, u64 z, u32_argb Color)
{
    Note->Rect  = Rect;
    Note->z     = z;
    Note->Color = Color;
}

isa_internal void
DrawRect(scn_offscreen_buffer Buffer, v2 Min, v2 Max, u32_argb Color)
{
    i32 StartX = RoundFloatToi32(Min.x);
    i32 StartY = RoundFloatToi32(Min.y);
    i32 EndX   = RoundFloatToi32(Max.x);
    i32 EndY   = RoundFloatToi32(Max.y);

    // Ensure StartX, StartY, EndX, EndY are within the buffer bounds
    if(StartX < 0)
    {
        StartX = 0;
    }
    if(StartY < 0)
    {
        StartY = 0;
    }
    if(EndX > Buffer.w)
    {
        EndX = (i32)Buffer.w;
    }
    if(EndY > Buffer.h)
    {
        EndY = (i32)Buffer.h;
    }

    i64 Pitch = Buffer.w * Buffer.BytesPerPixel;
    u8 *Row   = ((u8 *)Buffer.Mem) + (StartY * Pitch) + (StartX * Buffer.BytesPerPixel);

    for(i64 y = StartY; y < EndY; ++y)
    {
        u32 *Pixel = (u32 *)Row;
        for(i64 x = StartX; x < EndX; ++x)
        {
            *Pixel++ = Color.U32;
        }

        Row += Pitch;
    }
}

// NOTE(ingar): Casey says that your code should not be split up in this way the code that updates state and then
// renders should be executed simultaneously so we might want to do that
// NOTE(ingar): This was also in the context of games. Sinuce we're a traditional app we might have different needs to
// handle events asynchronously
extern "C" RESPOND_TO_MOUSE(RespondToMouse)
{
    scn_state *ScnState = InitScnState(Mem);

    // TODO(ingar): Convert to switch
    if(Event.Type == ScnMouseEvent_LDown)
    {
        MouseHistory.LClicked      = true;
        MouseHistory.PrevLClickPos = V2(Truncatei64ToFloat(Event.x), Truncatei64ToFloat(Event.y));
    }
    else if(Event.Type == ScnMouseEvent_RDown)
    {
        MouseHistory.RClicked = true;
    }

    // TODO(ingar): Make sure that int->float->int conversions actually function correctly. I think there are
    // inaccuracies as it is now
    if(Event.Type == ScnMouseEvent_LUp)
    {
        if(MouseHistory.Prev.Type == ScnMouseEvent_Move)
        {
            float PrevX = MouseHistory.PrevLClickPos.x;
            float PrevY = MouseHistory.PrevLClickPos.y;

            // TODO(ingar): The min v2 is not initialized properly. It is always set to 0,0
            rect NewRect = { V2(0, 0), V2(0, 0) };

            if(PrevX <= Event.x)
            {
                NewRect.min.x = (float)PrevX;
                NewRect.max.x = (float)Event.x;
            }
            else
            {
                NewRect.min.x = (float)Event.x;
                NewRect.max.x = (float)PrevX;
            }

            if(PrevY <= Event.y)
            {
                NewRect.min.y = (float)PrevY;
                NewRect.max.y = (float)Event.y;
            }
            else
            {
                NewRect.min.y = (float)Event.y;
                NewRect.max.y = (float)PrevY;
            }

            // TODO(ingar): Since the functions are ran on timers, this
            // probably means that we need synchronization mechanisms so that
            // new elements are not pushed simultaneously with the drawing

            if(ScnState->Notes->Count < ScnState->Notes->MaxCount)
            {
                FillNote(&ScnState->Notes->N[ScnState->Notes->Count++], NewRect, ScnState->Notes->z++,
                         U32Argb(255, 255, 255, 255));
            }
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
    scn_state *ScnState = InitScnState(Mem);

    /* Draw background */
    // TODO(ingar): Check if background is already filled and skip drawing it?
    DrawRect(Buffer, V2(0.0f, 0.0f), V2(Truncatei64ToFloat(Buffer.w), Truncatei64ToFloat(Buffer.h)), Bg.Color);

    for(u64 i = 0; i < ScnState->Notes->Count; ++i)
    {
        note Note = ScnState->Notes->N[i];

        // TODO(ingar): Same check as with the background?
        DrawRect(Buffer, Note.Rect.min, Note.Rect.max, Note.Color);
    }
}
