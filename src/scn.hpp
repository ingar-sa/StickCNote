/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
*/


#pragma once

#include "isa.hpp"
#include "utils.hpp"

struct scn_mem
{
    bool Initialized;

    size_t   PermanentMemSize;
    void *Permanent;

    size_t   WorkMemSize;
    void *Work;
};

struct offscreen_buffer
{
    i64 w;
    i64 h;
    i64 BytesPerPixel;

    void *Mem;
};

enum mouse_event
{
    MOUSE_LDOWN,
    MOUSE_LUP,
    MOUSE_RDOWN,
    MOUSE_RUP,

    MOUSE_MOVE,

    MOUSE_INVALID,
};


union u32_argb
{
    struct
    {
        u8 B, G, R, A;
    };
     
    u8 BGRA[4];
    u32 U32;
};


#define UPDATE_BACK_BUFFER(name) void name(offscreen_buffer Buffer)
typedef UPDATE_BACK_BUFFER(update_back_buffer);

extern "C" UPDATE_BACK_BUFFER(UpdateBackBufferStub)
{
    DebugPrint("UpdateBackBufferStub was called!\n");
}

#define RESPOND_TO_MOUSE(name) void name(enum mouse_event Event, i64 x, i64 y)
typedef RESPOND_TO_MOUSE(respond_to_mouse);
extern "C" RESPOND_TO_MOUSE(RespondToMouseStub)
{
    DebugPrint("RespondToMouseStub was called!\n");
}
