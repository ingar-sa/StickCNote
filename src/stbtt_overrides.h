#ifndef STBTT_OVERRIDES_H_
#define STBTT_OVERRIDES_H_

#include "isa.h"

struct stbtt_alloc_ctx
{
    isa_arena *Arena;
};

void *
StbttMalloc(size_t Size, stbtt_alloc_ctx *Ctx)
{
    return IsaArenaPush(Ctx->Arena, Size);
}

// TODO(ingar): lmao this aint gonna work. This only works if one allocation is made at a time.
void
StbttFree(void *Pointer, stbtt_alloc_ctx *Ctx)
{
    (void)Pointer;
    IsaArenaClear(Ctx->Arena);
}

#define STBTT_malloc(x, u) StbttMalloc(x, (stbtt_alloc_ctx *)u)
#define STBTT_free(x, u)   StbttFree(x, (stbtt_alloc_ctx *)u)

#endif // STBTT_OVERRIDES_H_
