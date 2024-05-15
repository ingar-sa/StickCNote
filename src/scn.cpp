/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
 */

#include "isa.h"
#include <cstdint>
#include <cstring>

ISA_LOG_REGISTER(Scn);

#include "win32/resources.h"
#include "win32/win32_utils.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
// #include "stbtt_overrides.h" // TODO(ingar): Need to make an allocator that will be able to replace malloc
#include "libs/stb_truetype.h"

#include "consts.h"
#include "scn_math.h"
#include "scn_intrinsics.h"
#include "scn.h"

isa_internal scn_state *
InitScnState(scn_mem *Mem)
{
    scn_state *State = (scn_state *)Mem->Permanent;
    if(!Mem->Initialized)
    {
        // TODO(ingar): It would be nice to create a system where switching between pushing something onto the permanent
        // or session arena could be done by changing something in a single place
        State->PermArena
            = IsaArenaCreate((u8 *)Mem->Permanent + sizeof(scn_state), Mem->PermanentMemSize - sizeof(scn_state));
        State->SessionArena = IsaArenaCreate((u8 *)Mem->Session, Mem->SessionMemSize);

        note_collection *NoteCollection = IsaPushStructZero(&State->PermArena, note_collection);
        NoteCollection->MaxCount        = 1024;
        NoteCollection->N               = IsaPushArray(&State->PermArena, note, NoteCollection->MaxCount);
        State->Notes                    = NoteCollection;

        State->MouseHistory = IsaPushStructZero(&State->PermArena, mouse_history);

        State->Stbtt          = IsaPushStructZero(&State->SessionArena, stbtt_ctx);
        State->Stbtt->MemSize = 2 << 25; // NOTE(ingar): Taken from one of the example programs
        State->Stbtt->Arena   = IsaPushStructZero(&State->SessionArena, isa_arena);
        IsaArenaCreate(State->Stbtt->Arena, IsaArenaPush(&State->SessionArena, State->Stbtt->MemSize),
                       State->Stbtt->MemSize);

        Mem->Initialized = true;
    }

    return State;
}

isa_internal void
FillNote(note *Note, rect Rect, u64 z, u32_argb Color)
{
    Note->Rect  = Rect;
    Note->z     = z;
    Note->Color = Color;
}

isa_internal void
DrawRect(scn_offscreen_buffer Buffer, v2 Min, v2 Max, u32_argb Color)
{
    i64 StartX = RoundFloatToi64(Min.x);
    i64 StartY = RoundFloatToi64(Min.y);
    i64 EndX   = RoundFloatToi64(Max.x);
    i64 EndY   = RoundFloatToi64(Max.y);

    Clamp(StartX, 0LL, StartX);
    Clamp(StartY, 0LL, StartY);
    Clamp(EndX, EndX, Buffer.w);
    Clamp(EndY, EndY, Buffer.h);

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
    scn_state       *ScnState     = InitScnState(Mem);
    mouse_history   *MouseHistory = ScnState->MouseHistory;
    note_collection *Notes        = ScnState->Notes;

    if(Event.Type == ScnMouseEvent_LDown)
    {
        MouseHistory->PrevLClickPos = V2(Truncatei64ToFloat(Event.x), Truncatei64ToFloat(Event.y));
    }
    else if(Event.Type == ScnMouseEvent_RDown)
    {
        MouseHistory->PrevRClickPos = V2(Truncatei64ToFloat(Event.x), Truncatei64ToFloat(Event.y));
    }
    else if(Event.Type == ScnMouseEvent_LUp)
    {
        /* Select note */
        if(MouseHistory->Prev.Type == ScnMouseEvent_LDown)
        {
            bool ClickedOnRect = false;
            /* Reverse iteration because we want the top-most note within the coordinates */
            for(i64 i = Notes->Count - 1; i >= 0; --i)
            {
                note *Note    = Notes->N + i;
                ClickedOnRect = InRect(Note->Rect, (float)Event.x, (float)Event.y);
                if(ClickedOnRect)
                {
                    Notes->NoteIsSelected = (Notes->SelectedNote == Note);
                    Notes->SelectedNote   = Note;
                    break;
                }
            }
        }

        MouseHistory->LClicked = true;
        MouseHistory->RClicked = false;
    }
    else if(Event.Type == ScnMouseEvent_RUp)
    {
        if(MouseHistory->Prev.Type == ScnMouseEvent_Move)
        {
            float PrevX   = MouseHistory->PrevRClickPos.x;
            float PrevY   = MouseHistory->PrevRClickPos.y;
            rect  NewRect = { V2(0, 0), V2(0, 0) };

            if(PrevX <= Event.x)
            {
                NewRect.Min.x = (float)PrevX;
                NewRect.Max.x = (float)Event.x;
            }
            else
            {
                NewRect.Min.x = (float)Event.x;
                NewRect.Max.x = (float)PrevX;
            }

            if(PrevY <= Event.y)
            {
                NewRect.Min.y = (float)PrevY;
                NewRect.Max.y = (float)Event.y;
            }
            else
            {
                NewRect.Min.y = (float)Event.y;
                NewRect.Max.y = (float)PrevY;
            }

            // TODO(ingar): Since the functions are ran on timers, this
            // probably means that we need synchronization mechanisms so that
            // new elements are not pushed simultaneously with the drawing

            if(Notes->Count < Notes->MaxCount)
            {
                // TODO(ingar): Bake the note number as text into the note and scale it to the note's size
                u64 z = Notes->Count++;
                FillNote(Notes->N + z, NewRect, z, U32Argb(GetRandu32()));
            }
        }

        MouseHistory->RClicked = true;
        MouseHistory->LClicked = false;
    }

    MouseHistory->Prev = Event;
}

extern "C" RESPOND_TO_KEYBOARD(RespondToKeyboard)
{
    scn_state       *ScnState = InitScnState(Mem);
    note_collection *Notes    = ScnState->Notes;

    IsaLogInfo("Key %lu was pressed", Event.Type);
    switch(Event.Type)
    {
        case ScnKeyboardEvent_A:
            break;
        case ScnKeyboardEvent_B:
            break;
        case ScnKeyboardEvent_C:
            {
                IsaLogInfo("C was pressed");

                Notes->Count          = 0;
                Notes->NoteIsSelected = false;
                Notes->SelectedNote   = nullptr;
            }
            break;
        case ScnKeyboardEvent_D:
            {
                IsaLogInfo("D was pressed");

                if(Notes->NoteIsSelected)
                {
                    note *ToDelete = Notes->SelectedNote;
                    if(Notes->Count > 1 && (ToDelete->z < (Notes->Count - 1)))
                    {
                        for(i64 i = ToDelete->z; i < (i64)Notes->Count; ++i)
                        {
                            memcpy(Notes->N + i, Notes->N + (i + 1), sizeof(note));
                            (Notes->N + i)->z--;
                        }
                    }

                    Notes->Count--;
                    Notes->NoteIsSelected = false;
                    Notes->SelectedNote   = nullptr;
                }
            }
            break;
        case ScnKeyboardEvent_E:
            break;
        case ScnKeyboardEvent_F:
            break;
        case ScnKeyboardEvent_G:
            break;
        case ScnKeyboardEvent_H:
            break;
        case ScnKeyboardEvent_I:
            break;
        case ScnKeyboardEvent_J:
            break;
        case ScnKeyboardEvent_K:
            break;
        case ScnKeyboardEvent_L:
            break;
        case ScnKeyboardEvent_M:
            break;
        case ScnKeyboardEvent_N:
            break;
        case ScnKeyboardEvent_O:
            break;
        case ScnKeyboardEvent_P:
            break;
        case ScnKeyboardEvent_Q:
            break;
        case ScnKeyboardEvent_R:
            break;
        case ScnKeyboardEvent_S:
            break;
        case ScnKeyboardEvent_T:
            break;
        case ScnKeyboardEvent_U:
            break;
        case ScnKeyboardEvent_V:
            break;
        case ScnKeyboardEvent_W:
            break;
        case ScnKeyboardEvent_X:
            break;
        case ScnKeyboardEvent_Y:
            break;
        case ScnKeyboardEvent_Z:
            break;

        case ScnKeyboardEvent_0:
            break;
        case ScnKeyboardEvent_1:
            break;
        case ScnKeyboardEvent_2:
            break;
        case ScnKeyboardEvent_3:
            break;
        case ScnKeyboardEvent_4:
            break;
        case ScnKeyboardEvent_5:
            break;
        case ScnKeyboardEvent_6:
            break;
        case ScnKeyboardEvent_7:
            break;
        case ScnKeyboardEvent_8:
            break;
        case ScnKeyboardEvent_9:
            break;

        case ScnKeyboardEvent_Shift:
            break;
        case ScnKeyboardEvent_Control:
            break;
        case ScnKeyboardEvent_Spacebar:
            break;
        case ScnKeyboardEvent_Alt:
            break;
        case ScnKeyboardEvent_Back:
            break;
        case ScnKeyboardEvent_Tab:
            break;
        case ScnKeyboardEvent_Enter:
            break;

        case ScnKeyboardEvent_Unhandled:
            {
                IsaLogError("Unhandled keyboard event was triggered");
            }
            break;
        default:
            {
                IsaAssert(0, "This should never happen!");
            }
            break;
    }
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
    DrawRect(Buffer, V2(0.0f, 0.0f), V2(Truncatei64ToFloat(Buffer.w), Truncatei64ToFloat(Buffer.h)),
             U32Argb(SCN_BG_COLOR));

    for(u64 i = 0; i < ScnState->Notes->Count; ++i)
    {
        note Note = ScnState->Notes->N[i];

        // TODO(ingar): Same check as with the background?
        DrawRect(Buffer, Note.Rect.Min, Note.Rect.Max, Note.Color);
    }
}
