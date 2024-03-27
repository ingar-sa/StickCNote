/*
 * Copyright 2024 (c) by Ingar Solveigson Asheim. All Rights Reserved.
*/


#include "isa.hpp"
#include "mem.hpp"
 // TODO(ingar): More error handling and null-pointer checks


void
InitMemArena(mem_arena *Arena, void *Mem, size_t Size)
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
ArenaAlloc_(mem_arena *Arena, size_t Size, size_t Alignment)
{
    IsaAssert((Alignment & (Alignment - 1)) == 0);

    size_t Adjustment = (Alignment - (Arena->Offset + (Alignment - 1)) & (Alignment - 1));
    IsaAssert(Arena->Offset + Adjustment + Size <= Arena->Size);

    Arena->Offset += Adjustment;

    void *AlignedMemStart = (void *)(Arena->Start + Arena->Offset);
    Arena->Offset += Size; // Increase the offset by the size of the allocation

    return AlignedMemStart;
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

    size_t ElemSize;
    u64 ElemCap;
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
CreateMemPool(mem_pool *Pool, void *Mem, size_t MemSize,
              size_t ElemSize, u64 ElemCap)
{
    // Assuming ElemSize is set to the size of the type, round it up to meet alignment requirements
    size_t Alignment = alignof(max_align_t); // Or specific type alignment if known
                                             //
    Pool->ElemSize = (ElemSize + Alignment - 1) & ~(Alignment - 1);
    Pool->ElemCap = ElemCap;

    size_t AvailabilityOverviewSize = ElemCap; // NOTE(ingar): Assert this is in bytes 
    size_t PoolSize = ElemCap * Pool->ElemSize;

    IsaAssert(AvailabilityOverviewSize + PoolSize == MemSize);
    
    Pool->MemSize = MemSize;
    Pool->MemStart = (uintptr_t)Mem;

    Pool->AvailabilityOverview = (u8 *)Mem;

    for(u64 i = 0; i < ElemCap; ++i)
    {
        ((u8 *)Mem)[i] = 1; 
    }
    
    Pool->PoolSize = MemSize - AvailabilityOverviewSize;
    Pool->PoolStart = (uintptr_t)Mem + AvailabilityOverviewSize;// TODO(ingar): Assure this comes out as a addition of bytes 

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
PoolDefrag(mem_pool *Pool)
{

}

void
PoolAlloc(mem_pool *Pool, pool_alloc *Alloc)
{
    u64 AvailableCount = 0;
    bool FoundSpace = false;

    for(u64 i = 0; i < Pool->ElemCap; ++i)
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
           Alloc->FirstElemIdx= i - Alloc->NElems;

           break;
        }

        if(i == (Pool->ElemSize - 1))
        {
            PoolDefrag(Pool);
        }
    }

    if(!FoundSpace)
    {
        Alloc->Mem = nullptr;
        return;
    }

    for(u64 i = 0; i < Alloc->NElems; ++i)
    {
        u64 Idx = Alloc->FirstElemIdx + i;
        Pool->AvailabilityOverview[Idx] = 0;
    }

    size_t AllocStartOffset = Alloc->FirstElemIdx * Pool->ElemSize;
    Alloc->Mem = (void *)(Pool->PoolStart + AllocStartOffset);
}

void
PoolDealloc(mem_pool *Pool, pool_alloc *Alloc)
{
    for(u64 i = 0; i < Alloc->NElems; ++i)
    {
        u64 Idx = Alloc->FirstElemIdx + i; 
        Pool->AvailabilityOverview[Idx] = 1;
    }
}

