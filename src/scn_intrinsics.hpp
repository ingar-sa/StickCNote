#pragma once

#include "isa.hpp"

#include "math.h"

inline i32
SignOfi32(i32 Value)
{
    i32 Result = (Value >= 0) ? 1 : -1;
    return Result;
}

inline float
SquareRoot(float Float)
{
    float Result = sqrtf(Float);
    return Result;
}

inline float
AbsoluteValue(float Float)
{
    float Result = fabs(Float);
    return Result;
}

inline u32
RotateLeft(u32 Value, i32 Amount)
{
#if COMPILER_MSVC
    u32 Result = _rotl(Value, Amount);
#else
    Amount &= 31;
    u32 Result = ((Value << Amount) | (Value >> (32 - Amount)));
#endif

    return Result;
}

inline u32
RotateRight(u32 Value, i32 Amount)
{
#if COMPILER_MSVC
    u32 Result = _rotr(Value, Amount);
#else
    Amount &= 31;
    u32 Result = ((Value >> Amount) | (Value << (32 - Amount)));
#endif

    return Result;
}

inline i32
RoundFloatToi32(float Float)
{
    i32 Result = (i32)roundf(Float);
    return Result;
}

inline u32
RoundFloatTou32(float Float)
{
    u32 Result = (u32)roundf(Float);
    return Result;
}

inline i32 
FloorFloatToi32(float Float)
{
    i32 Result = (i32)floorf(Float);
    return Result;
}

inline i32 
CeilFloatToi32(float Float)
{
    i32 Result = (i32)ceilf(Float);
    return Result;
}

inline i32
TruncateFloatToi32(float Float)
{
    i32 Result = (i32)Float;
    return Result;
}

inline i64
RoundFloatToi64(float Float)
{
    i64 Result = (i64)roundf(Float);
    return Result;
}

inline u64
RoundFloatTou64(float Float)
{
    u64 Result = (u64)roundf(Float);
    return Result;
}

inline i64 
FloorFloatToi64(float Float)
{
    i64 Result = (i64)floorf(Float);
    return Result;
}

inline i64 
CeilFloatToi64(float Float)
{
    i64 Result = (i64)ceilf(Float);
    return Result;
}

inline i64
TruncateFloatToi64(float Float)
{
    i64 Result = (i64)Float;
    return Result;
}
