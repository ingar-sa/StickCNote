#pragma once

#include "isa.hpp"
#include "scn_math.hpp"

// TODO(ingar): Casey uses v3 and v4 to represent colors in float space.
// Is this a better approach?
union u32_argb
{
    struct
    {
        u8 b, g, r, a;
    };
     
    u8 BGRA[4];
    u32 U32;
};

struct gui_rect
{
    rect Dim;
    u32_argb Color;
};
