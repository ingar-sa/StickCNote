/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
 */

#ifndef SCN_H_
#define SCN_H_

#include "consts.h"
#include "isa.h"
#include "scn_math.h"
#include "stbtt_overrides.h"

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
    i64 w;
    i64 h;
    i64 BytesPerPixel;

    void *Mem;
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

struct gui_rect
{
    rect     Dim;
    u32_argb Color;
};

isa_global struct bg_landscape
{
    u32_argb Color = { .U32 = (u32)PRUSSIAN_BLUE };
    i64      x, y;

} Bg;

// TODO(ingar): NOTE to self. When dragging, there should be a partially transparent rectangle that shows what the note
// will look like. There should also be a simple color picker, and you could adjust the opacity (or something else) by
// scrolling while choosing the color.
struct note
{
    rect     Rect;
    u64      z;
    u32_argb Color;
};

struct note_collection
{
    u64   MaxCount;
    u64   Count;
    u64   z;
    note *N;
};

// TODO(ingar): Figure out how much memory stbtt uses. In the example programs 2^20 to 2^25 bytes are used.
struct stbtt_ctx
{
    isa_arena *Arena;
};

// NOTE(ingar): The items in the state that require a "substantial amount of memory will be pushed onto one of the
// arenas instead of being part of the struct
struct scn_state
{
    isa_arena        PermArena;
    note_collection *Notes;

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
