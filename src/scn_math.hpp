#pragma once

#include "isa.hpp"
#include "scn_intrinsics.hpp"

union v2
{
    struct
    {
        float x, y;
    };
    float E[2];
};

struct rect
{
    v2 min, max;
};

typedef v2 p2;

inline v2
V2(float x, float y)
{
    v2 Result;

    Result.x = x;
    Result.y = y;

    return Result;
}

inline v2
operator+(v2& a, v2& b)
{
    v2 Result;
    Result.x = a.x + b.x;
    Result.y = a.y + b.y;
    return Result;
}

inline v2
operator-(v2& a, v2& b)
{
    v2 Result;
    Result.x = a.x - b.x;
    Result.y = a.y - b.y;
    return Result;
}

inline v2
operator-(v2& a)
{
    a.x = -a.x;
    a.y = -a.y;
    return a;
}

inline bool
InRect(float x, float y, rect r)
{
    bool InX = (x >= r.min.x) && (x <= r.max.x);
    bool InY = (y >= r.min.y) && (y <= r.max.y);

    return (InX && InY);
}

inline u32 *
GetPcgState_(void)
{
    isa_persist u32 PcgState = 0;
    return &PcgState;
}

inline void
SeedRandPcg_(u32 Seed)
{
    *GetPcgState_() = Seed;
}

inline u32
GetRandu32(void)
{
    uint32_t State = *GetPcgState_();
    *GetPcgState_() = State * 747796405u + 2891336453u;
    uint32_t Word = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
    return (Word >> 22u) ^ Word;   
}

inline u32
GetRandu32InRange(u32 Min, u32 Max)
{
    double Scalar = (double)GetRandu32() / (double)(UINT32_MAX);
    return (u32)(Min + (Scalar * (Max - Min)));
}
