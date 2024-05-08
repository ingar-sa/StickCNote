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

        // TODO(ingar): Make the pool a part of the state struct instead of allocating it through the arena?
        const u32        MAX_NOTES      = 1024;
        note_collection *NoteCollection = IsaPushStructZero(&State->Arena, note_collection);
        isa_arena       *NoteArena      = IsaPushStructZero(&State->Arena, isa_arena);
        NoteCollection->MaxCount        = MAX_NOTES;
        NoteCollection->Arena           = NoteArena;
        State->Notes                    = NoteCollection;

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

            rect NewRect = { V2(0, 0), V2(0, 0) };

            if(PrevX < Event.x)
            {
                NewRect.min.x = (float)PrevX;
                NewRect.max.x = (float)Event.x;
            }
            else
            {
                NewRect.min.x = (float)Event.x;
                NewRect.max.x = (float)PrevX;
            }

            if(PrevY < Event.x)
            {
                NewRect.min.y = (float)PrevY;
                NewRect.max.y = (float)Event.y;
            }
            else
            {
                NewRect.min.y = (float)Event.y;
                NewRect.max.y = (float)PrevY;
            }

            // TODO(ingar): We need to push ui elements into memory which
            // UpdateBackBuffer then iterates over and draws
            // TODO(ingar): Since the functions are ran on timers, this
            // probably means that we need synchronization mechanisms so that
            // new elements are not pushed simultaneously with the drawing

            // TODO(ingar): Add bounds checking for z
            note *NewNote = IsaPushStructZero(ScnState->Notes->Arena, note);
            FillNote(NewNote, NewRect, ScnState->Notes->CurZ++, U32Argb(255, 0, 0, 255));
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

    note *Notes = (note *)ScnState->Notes->Arena->Mem;
    for(u64 i = 0; i < ScnState->Notes->Count; ++i)
    {
        note Note = Notes[i];

        DrawRect(Buffer, Note.Rect.min.x, Note.Rect.min.y, Note.Rect.max.x - Note.Rect.min.x,
                 Note.Rect.max.y - Note.Rect.min.y, Note.Color);
    }
}
