/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
*/


#pragma once

#include "isa.h"

struct scn_mem
{
    bool Initialized;

    size_t PermanentMemSize;
    void *Permanent;

    size_t SessionMemSize;
    void *Session;
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
    i64 x, y;
};


// TODO(ingar): In order to reserve arenas for different purposes, we need to
// keep track of where each arena is on the stack, as well as the next point
// in memory that a new arena can be allocated is.
// Due to the nature of arenas, we are limiting the number of items that can be
// added when creating the arena. Since we technically should be able to handle
// an "infinite" amount of entities, we have to find a way to support this.
struct scn_state
{
    isa_mem_arena Arena;
};

// TODO(ingar): Add (and figure out what it is) thread context
#define UPDATE_BACK_BUFFER(name) void name(scn_mem Mem, scn_offscreen_buffer Buffer)
typedef UPDATE_BACK_BUFFER(update_back_buffer);

extern "C" UPDATE_BACK_BUFFER(UpdateBackBufferStub)
{
    //DebugPrint("UpdateBackBufferStub was called!\n");
    assert(0 /*UpdateBackBufferStub was called!*/);
}

#define RESPOND_TO_MOUSE(name) void name(scn_mem Mem, scn_mouse_event Event)
typedef RESPOND_TO_MOUSE(respond_to_mouse);
extern "C" RESPOND_TO_MOUSE(RespondToMouseStub)
{
    //DebugPrint("RespondToMouseStub was called!\n");
    assert(0 /*RespondToMouseStub was called!*/);
}

// TODO(ingar): Is this way of doing this overkill?
// NOTE(ingar): This really seems like overkill for this.
// NOTE(ingar): This is overkill
#define SEED_RAND_PCG(name) void name(u32 Seed)
typedef SEED_RAND_PCG(seed_rand_pcg);
extern "C" SEED_RAND_PCG(SeedRandPcgStub)
{
    //DebugPrint("RespondToMouseStub was called!\n");
    assert(0 /*SeedRandPcgStub was called!*/);
}
