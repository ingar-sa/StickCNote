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
// #include "stbtt_overrides.h"
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
        State->PermArena
            = IsaArenaCreate((u8 *)Mem->Permanent + sizeof(scn_state), Mem->PermanentMemSize - sizeof(scn_state));
        State->SessionArena = IsaArenaCreate((u8 *)Mem->Session, Mem->SessionMemSize);

        note_collection *NoteCollection = IsaPushStructZero(&State->PermArena, note_collection);
        NoteCollection->MaxCount        = 1024;
        NoteCollection->N               = IsaPushArray(&State->PermArena, note, NoteCollection->MaxCount);
        State->Notes                    = NoteCollection;

        State->MouseHistory = IsaPushStructZero(&State->PermArena, mouse_history);

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
                    u64 Index = Notes->SelectedNote->z;
                    IsaArrayDeleteAndShift(Notes->N, Index, Notes->Count, sizeof(note));

                    Notes->Count--;
                    Notes->NoteIsSelected = false;
                    Notes->SelectedNote   = nullptr;

                    for(u64 i = Index; i < Notes->Count; ++i)
                    {
                        Notes->N[i].z--;
                    }
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

isa_internal void
DrawText(isa_arena *Arena, scn_offscreen_buffer Buffer)
{
    IsaArenaF5(Arena);

    isa_file_data *FontFile = IsaLoadFileIntoMemory("c:/windows/fonts/arialbd.ttf");

    stbtt_fontinfo Font;
    stbtt_InitFont(&Font, FontFile->Data, 0);

    float Scalar = stbtt_ScaleForPixelHeight(&Font, 50.0f);

    int Ascent, Descent, LineGap;
    stbtt_GetFontVMetrics(&Font, &Ascent, &Descent, &LineGap);

    float XPos     = 2;
    int   Baseline = (int)(Ascent * Scalar);

    isa_string    Text = IsaNewString("Heljo World!");
    const u64     BufY = 60;
    const u64     BufX = 240;
    unsigned char TTBuf[BufY][BufX];

    for(u64 i = 0; i < Text.Len; ++i)
    {
        int   Advance, Lsb, x0, y0, x1, y1;
        float XShift = XPos - (float)FloorFloatToi64(XPos);

        stbtt_GetCodepointHMetrics(&Font, (int)Text.String[i], &Advance, &Lsb);

        stbtt_GetCodepointBitmapBoxSubpixel(&Font, (int)Text.String[i], Scalar, Scalar, XShift, 0, &x0, &y0, &x1, &y1);

        stbtt_MakeCodepointBitmapSubpixel(&Font, &TTBuf[Baseline + y0][(int)XPos + x0], x1 - x0, y1 - y0, BufY, Scalar,
                                          Scalar, XShift, 0, Text.String[i]);
        XPos += (Advance * Scalar);
        if(i < (Text.Len - 1))
        {
            XPos += Scalar * stbtt_GetCodepointKernAdvance(&Font, (int)Text.String[i], (int)Text.String[i + 1]);
        }
    }

    i64 StartY = 200;
    i64 StartX = 200;

    i64 Pitch = Buffer.w * Buffer.BytesPerPixel;
    u8 *Row   = ((u8 *)Buffer.Mem) + (StartY * Pitch) + (StartX * Buffer.BytesPerPixel);

    for(u64 y = 0; y < BufY; ++y)
    {
        u32 *Pixel = (u32 *)Row;
        for(u64 x = 0; x < BufX; ++x)
        {
            *Pixel++ = TTBuf[y][x];
        }

        Row += Pitch;
    }

    free(FontFile);
    IsaArenaF9(Arena);
}

isa_internal void
DrawChar(isa_arena *Arena, scn_offscreen_buffer Buffer)
{
    IsaArenaF5(Arena);

    isa_file_data *FontFile = IsaLoadFileIntoMemory("c:/windows/fonts/arialbd.ttf");

    stbtt_fontinfo Font;
    stbtt_InitFont(&Font, FontFile->Data, 0);

    isa_string Text = IsaNewString("Thank God it worked!");

    i64 StartY = 200;
    i64 StartX = 200;

    for(u64 i = 0; i < Text.Len; ++i)
    {
        int            w, h;
        unsigned char *Bitmap
            = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 50.0f), Text.String[i], &w, &h, 0, 0);

        i64 Pitch = Buffer.w * Buffer.BytesPerPixel;
        u8 *Row   = ((u8 *)Buffer.Mem) + (StartY * Pitch) + (StartX * Buffer.BytesPerPixel);

        for(u64 y = 0; y < h; ++y)
        {
            u32 *Pixel = (u32 *)Row;
            for(u64 x = 0; x < w; ++x)
            {
                *Pixel++ = Bitmap[(y * w) + x];
            }

            Row += Pitch;
        }
        StartX += w + 5;

        stbtt_FreeBitmap(Bitmap, NULL);
    }

    free(FontFile);
    IsaArenaF9(Arena);
}

extern "C" UPDATE_BACK_BUFFER(UpdateBackBuffer)
{
    scn_state *ScnState = InitScnState(Mem);

    static bool Draw = true;
    if(Draw)
    {
        /* Draw background */
        // TODO(ingar): Check if background is already filled and skip drawing it?
        DrawRect(Buffer, V2(0.0f, 0.0f), V2(Truncatei64ToFloat(Buffer.w), Truncatei64ToFloat(Buffer.h)),
                 U32Argb(SCN_BG_COLOR));

        // DrawText(&ScnState->SessionArena, Buffer);
        DrawChar(&ScnState->SessionArena, Buffer);
        for(u64 i = 0; i < ScnState->Notes->Count; ++i)
        {
            // note Note = ScnState->Notes->N[i];
            //  TODO(ingar): Same check as with the background?
            // DrawRect(Buffer, Note.Rect.Min, Note.Rect.Max, Note.Color);
        }
        Draw = false;
    }
}
