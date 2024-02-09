#pragma once

#include "isa.hpp"
#include "utils.hpp"

struct app_mem
{
    bool Initialized;

    u64   PermanentMemSize;
    void *Permanent;

    u64   WorkMemSize;
    void *Work;
};

struct offscreen_buffer
{
    i64 w;
    i64 h;
    i64 BytesPerPixel;

    void *Mem;
};

#define RESPOND_TO_MOUSE_CLICK(name) void name(offscreen_buffer Buffer)
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


