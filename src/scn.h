/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
 */

#ifndef SCN_H_
#define SCN_H_

#include "consts.h"
#include "isa.h"
#include "scn_math.h"

ISA_LOG_DECLARE_SAME_TU;

struct scn_mem
{
    bool Initialized;

    size_t PermanentMemSize;
    void  *Permanent;

    size_t SessionMemSize;
    void  *Session;
};

// TODO(ingar): Make the storage of this internal to scn instead of the
// pointer being passed in from the platform layer?
struct scn_offscreen_buffer
{
    i64 w, h;
    u64 BytesPerPixel;

    void *Mem;
};

enum scn_keyboard_event_type
{
    // Alphabet keys
    // NOTE(ingar): There are no concepts of lower-case letters in Windows' virtual keys
    // Virtual key overview: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    ScnKeyboardEvent_A,
    ScnKeyboardEvent_B,
    ScnKeyboardEvent_C,
    ScnKeyboardEvent_D,
    ScnKeyboardEvent_E,
    ScnKeyboardEvent_F,
    ScnKeyboardEvent_G,
    ScnKeyboardEvent_H,
    ScnKeyboardEvent_I,
    ScnKeyboardEvent_J,
    ScnKeyboardEvent_K,
    ScnKeyboardEvent_L,
    ScnKeyboardEvent_M,
    ScnKeyboardEvent_N,
    ScnKeyboardEvent_O,
    ScnKeyboardEvent_P,
    ScnKeyboardEvent_Q,
    ScnKeyboardEvent_R,
    ScnKeyboardEvent_S,
    ScnKeyboardEvent_T,
    ScnKeyboardEvent_U,
    ScnKeyboardEvent_V,
    ScnKeyboardEvent_W,
    ScnKeyboardEvent_X,
    ScnKeyboardEvent_Y,
    ScnKeyboardEvent_Z,

    // Number keys
    ScnKeyboardEvent_0,
    ScnKeyboardEvent_1,
    ScnKeyboardEvent_2,
    ScnKeyboardEvent_3,
    ScnKeyboardEvent_4,
    ScnKeyboardEvent_5,
    ScnKeyboardEvent_6,
    ScnKeyboardEvent_7,
    ScnKeyboardEvent_8,
    ScnKeyboardEvent_9,

    // Special keys
    ScnKeyboardEvent_Shift,
    ScnKeyboardEvent_Control,
    ScnKeyboardEvent_Spacebar,
    ScnKeyboardEvent_Alt,
    ScnKeyboardEvent_Back,
    ScnKeyboardEvent_Tab,
    ScnKeyboardEvent_Return,

    // Invalid key event
    ScnKeyboardEvent_Invalid,
};

struct scn_keyboard_event
{
    scn_keyboard_event_type Type;
};

enum scn_mouse_event_type
{
    ScnMouseEvent_LDown,
    ScnMouseEvent_LUp,
    ScnMouseEvent_RDown,
    ScnMouseEvent_RUp,

    ScnMouseEvent_Move,

    ScnMouseEvent_Invalid,
};

struct scn_mouse_event
{
    scn_mouse_event_type Type;
    i64                  x, y;
};

struct mouse_history
{
    bool LClicked;
    bool RClicked;

    scn_mouse_event Prev;
    scn_mouse_event PrevLClick;
    scn_mouse_event PrevRClick;

    v2 PrevLClickPos;
    v2 PrevRClickPos;
};

// TODO(ingar): Add/convert float-based colors
union u32_argb
{
    struct
    {
        u8 b, g, r, a;
    };

    u8  BGRA[4];
    u32 U32;
};

inline u32_argb
U32Argb(u8 b, u8 g, u8 r, u8 a)
{
    u32_argb Color = {
        { b, g, r, a }
    };
    return Color;
}

inline u32_argb
U32Argb(u32 U32)
{
    u32_argb Color = { .U32 = U32 };
    return Color;
}

struct gui_rect
{
    rect     Dim;
    u32_argb Color;
};

// TODO(ingar): NOTE to self. When dragging, there should be a partially transparent rectangle that shows what the note
// will look like. There should also be a simple color picker, and you could adjust the opacity (or something else) by
// scrolling while choosing the color.
struct note
{
    rect Rect;
    // TODO(ingar): Turn this into a u64?
    i64      z;
    u64      CollectionPos;
    u32_argb Color;
};

struct note_collection
{
    u64 MaxCount;
    u64 Count;
    i64 TopZ;

    note *SelectedNote;
    bool  NoteIsSelected;

    note *N;
};

// TODO(ingar): Figure out how much memory stbtt uses. In the example programs 2^20 to 2^25 bytes are used.
struct stbtt_ctx
{
    isa_arena *Arena;
    size_t     MemSize;
};

// NOTE(ingar): The items in the state that require a "substantial amount of memory will be pushed onto one of the
// arenas instead of being part of the struct
struct scn_state
{
    isa_arena        PermArena;
    note_collection *Notes;
    mouse_history   *MouseHistory;

    isa_arena  SessionArena;
    stbtt_ctx *Stbtt;
};

// TODO(ingar): Add (and figure out what it is) thread context
#define UPDATE_BACK_BUFFER(name) void name(scn_mem *Mem, scn_offscreen_buffer Buffer)
typedef UPDATE_BACK_BUFFER(update_back_buffer);

extern "C" UPDATE_BACK_BUFFER(UpdateBackBufferStub)
{
    // DebugPrint("UpdateBackBufferStub was called!\n");
    assert(0 /*UpdateBackBufferStub was called!*/);
}

#define RESPOND_TO_MOUSE(name) void name(scn_mem *Mem, scn_mouse_event Event)
typedef RESPOND_TO_MOUSE(respond_to_mouse);
extern "C" RESPOND_TO_MOUSE(RespondToMouseStub)
{
    // DebugPrint("RespondToMouseStub was called!\n");
    assert(0 /*RespondToMouseStub was called!*/);
}

#define RESPOND_TO_KEYBOARD(name) void name(scn_mem *Mem, scn_keyboard_event Event)
typedef RESPOND_TO_KEYBOARD(respond_to_keyboard);
extern "C" RESPOND_TO_KEYBOARD(RespondToKeyboardStub)
{
    assert(0 /*RespondToKeyboardStub was called!*/);
}
// TODO(ingar): Is this way of doing this overkill?
// NOTE(ingar): This really seems like overkill for this.
// NOTE(ingar): This is overkill
#define SEED_RAND_PCG(name) void name(u32 Seed)
typedef SEED_RAND_PCG(seed_rand_pcg);
extern "C" SEED_RAND_PCG(SeedRandPcgStub)
{
    // DebugPrint("RespondToMouseStub was called!\n");
    assert(0 /*SeedRandPcgStub was called!*/);
}

#endif // SCN_H_
