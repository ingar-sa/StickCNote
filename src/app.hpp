/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
*/


#pragma once

#include "isa.hpp"
#include "utils.hpp"

struct app_mem
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

    MOUSE_INVALID,
};

#define RESPOND_TO_MOUSE_HOVER(name) void name(i64 x, i64 y)
typedef RESPOND_TO_MOUSE_HOVER(respond_to_mouse_hover);

extern "C" RESPOND_TO_MOUSE_HOVER(RespondToMouseHoverStub)
{
    DebugPrint("RespondToMouseHoverStub was called!\n");
}

#define RESPOND_TO_MOUSE_CLICK(name) void name(enum mouse_event Event, i64 x, i64 y)
typedef RESPOND_TO_MOUSE_CLICK(respond_to_mouse_click);

extern "C" RESPOND_TO_MOUSE_CLICK(RespondToMouseClickStub)
{
    DebugPrint("RespondToMouseClickStub was called!\n");
}


#define UPDATE_BACK_BUFFER(name) void name(offscreen_buffer Buffer)
typedef UPDATE_BACK_BUFFER(update_back_buffer);

extern "C" UPDATE_BACK_BUFFER(UpdateBackBufferStub)
{
    DebugPrint("UpdateBackBufferStub was called!\n");
}


