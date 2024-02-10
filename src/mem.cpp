#include "isa.hpp"
#include "app.hpp"
#include <cstddef>

 // TODO(ingar): More error handling and null-pointer checks

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

void
DestroyMemArena(mem_arena **Arena)
{
    if(Arena == nullptr || *Arena == nullptr)
    {
        return;
    }

    *Arena = nullptr;
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
    size_t MemSize;
    uintptr_t MemStart;

    u8 *AvailabilityOverview;
    
    size_t PoolSize;
    uintptr_t PoolStart;
    size_t Offset;

    size_t ElemSize;
    u64 ElemCap;
    u64 ElemCount;

};

struct pool_alloc
{
    size_t ElemSize;
    u64 FirstElemIdx;
    u64 NElems;

    void *Mem;
};

// TODO(ingar): We need to make sure to add padding in order to meet alignment criteria 
void
CreateMemPool(mem_pool *Pool, void *Mem, size_t Size,
              size_t ElemSize, u64 ElemCap)
{
    // TODO(ingar): We should probably check if the element size is a multiple of size instead
    
    size_t AvailabilityOverviewSize = ElemCap; // NOTE(ingar): Assert this is in bytes 
    size_t PoolSize = ElemCap * ElemSize;

    IsaAssert(AvailabilityOverviewSize + PoolSize == Size);
    
    Pool->MemSize = Size;
    Pool->MemStart = (uintptr_t)Mem;

    Pool->AvailabilityOverview = (u8 *)Mem;

    for(u64 i = 0; i < ElemCap; ++i)
    {
        ((u8 *)Mem)[i] = 1; // Set all as available
    }
    
    Pool->PoolSize = Size - AvailabilityOverviewSize;
    Pool->PoolStart = (uintptr_t)Mem + AvailabilityOverviewSize;// TODO(ingar): Assure this comes out as a addition of bytes 
    Pool->Offset = 0;
    
    Pool->ElemSize = ElemSize;
    Pool->ElemCap = ElemCap;
    Pool->ElemCount = 0;
}

void
DestroyMemPool(mem_pool** Pool)
{
    if(Pool == nullptr || *Pool == nullptr)
    {
        return;
    }

    *Pool = nullptr;
}

void
PoolAlloc(mem_pool *Pool, pool_alloc *Alloc)
{
    u64 AvailableCount = 0;
    bool FoundSpace = false;
    u64 i = 0;

    for(; i < Pool->ElemCap; ++i)
    {
        if(Pool->AvailabilityOverview[i])
        {
            ++AvailableCount;
        }
        else if(!Pool->AvailabilityOverview[i])
        {
            AvailableCount = 0;
        }
            
        if(AvailableCount == Alloc->NElems)
        {
           FoundSpace = true;
           break;
        }
    }

    if(!FoundSpace)
    {
        Alloc->Mem = nullptr;
        return;
    }
    
    size_t AllocStartOffset = (i - Alloc->NElems) * Pool->ElemSize;
    Alloc->Mem = (void *)(Pool->PoolStart + AllocStartOffset);
}

void
PoolDealloc(mem_pool *Pool, pool_alloc *Alloc)
{
    for(u64 i = 0; i < Alloc->NElems; ++i)
    {
        u64 Idx = Alloc->FirstElemIdx;
    }
}

