
#include "isa.hpp"
#include "scn.hpp"
#include "consts.hpp"

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
    u8 *Row = ((u8 *)Buffer.Mem) + (Y * Pitch) + X;

    for(i64 y = Y; y < Y + Height; ++y)
    {
        u32 *Pixel = (u32 *)Row;
        for(i64 x = X; x < X + Width; ++x) {
            *Pixel++ = Color.U32;
        }

        Row += Pitch;
    }    
}
