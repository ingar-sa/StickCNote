#include "isa.hpp"
#include "app.hpp"
#include <cstddef>

struct mem_arena
{
    size_t Size;

    uintptr_t Start;
    size_t    Offset;
};

void
CreateMemArena(mem_arena *Arena, void *Mem, size_t Size)
{
    Arena->Size = Size;
    Arena->Start = (uintptr_t)Mem;
    Arena->Offset = 0;
}

void *
ArenaAlloc(mem_arena *Arena, size_t Size)
{
    IsaAssert(Arena->Offset + Size <= Arena->Size);

    void *MemStart = (void *)(Arena->Start + Arena->Offset);

    Arena->Offset += Size;

    return MemStart;
}

void
ArenaDealloc(mem_arena *Arena)
{
    Arena->Offset = 0;
}

struct mem_pool
{
    size_t Size;

    u8 *AvailabilityOverview;

    uintptr_t Start;
    size_t Offset;

    size_t ElemSize;
    u64 ElemCap;
    u64 ElemCount;

};

struct pool_alloc
{
    u64 NElems;
    uintptr_t Start;
    void *Mem;
};

// TODO(ingar): We need to make sure to add padding in order to meet alignment criteria 
void
CreateMemPool(mem_pool *Pool, void *Mem, size_t ElemSize, u64 ElemCap) 
{
    // TODO(ingar): We should probably check if the element size is a multiple of size instead

    Pool->AvailabilityOverview = (u8 *)Mem;

    for(u64 i = 0; i < ElemCap; ++i)
    {
        ((u8 *)Mem)[i] = 1; // Set all as available
    }

    Pool->Start = (uintptr_t)Mem + ElemCap;// TODO(ingar): Assure this comes out as a addition of bytes 
    Pool->Offset = 0;
    
    Pool->ElemSize = ElemSize;
    Pool->ElemCap = ElemCap;// TODO(ingar): Is this correct?
    Pool->ElemCount = 0;
}

void
PoolAlloc(mem_pool *Pool, pool_alloc *Alloc, u64 NElems)
{
    IsaAssert(Pool->ElemCount + NElems <= Pool->ElemCap);
    
    uintptr_t AllocStart = Pool->Start + Pool->Offset;
    void *MemStart = (void *)AllocStart;

    Pool->ElemCount += NElems;

    Alloc->NElems = NElems;
    Alloc->Start = AllocStart;
    Alloc->Mem = MemStart;
}

