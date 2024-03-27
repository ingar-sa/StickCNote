#pragma once

#include <stdint.h>

struct mem_arena
{
    size_t Size;

    uintptr_t Start;
    size_t    Offset;
};

#define PushStruct(Arena, type) \
    (type *)ArenaAlloc_(Arena, 1, alignof(type))

#define PushArray(Arena, Count, type) \
    (type *)ArenaAlloc_(Arena, Count * sizeof(type), alignof(type))

