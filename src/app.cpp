#include "isa.hpp"
#include "app.hpp"

extern "C" RESPOND_TO_MOUSE_CLICK(RespondToMouseClick)
{

}

// void name(offscreen_buffer Buffer)
extern "C" UPDATE_BACK_BUFFER(UpdateBackBuffer)
{
    i64 Pitch = Buffer.w * Buffer.BytesPerPixel;
    u8 *Row = (u8 *)Buffer.Mem;

    for(i64 y = 0; y < Buffer.h; ++y)
    {
        u32 *Pixel = (u32 *)Row;
        for(i32 x = 0; x < Buffer.w; ++x) {
            *Pixel++ = 0xff00ff;
        }

        Row += Pitch;
    }
} 
