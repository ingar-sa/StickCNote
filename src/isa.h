#ifndef ISA_H_

#if(defined(_WIN32) || defined(_WIN64)) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

/*
    TODO(ingar): Add error handling for initialization stuff
*/

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //NOTE(ingar): Replace with our own functions?


#if 0 // defined(_cplusplus)
extern "C"
{
#endif
////////////////////////////////////////
//              DEFINES               //
////////////////////////////////////////

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

////////////////////////////////////////
//                MISC                //
////////////////////////////////////////

#define KiloByte(Number) (Number * 1024ULL)
#define MegaByte(Number) (KiloByte(Number) * 1024ULL)
#define GigaByte(Number) (MegaByte(Number) * 1024ULL)
#define TeraByte(Number) (GigaByte(Number) * 1024ULL)

#define IsaArrayLen(Array) (sizeof(Array) / sizeof(Array[0]))

bool
IsaDoubleEpsilonCompare(const double A, const double B)
{
    double GreatestValue = (fabs(A) < fabs(B)) ? fabs(B) : fabs(A);
    return fabs(A - B) <= (GreatestValue * DBL_EPSILON);
}

uint64_t
IsaDoubleSignBit(double F)
{
    uint64_t  Mask = 1ULL << 63;
    uint64_t *Comp = (uint64_t *)&F;

    return (*Comp) & Mask;
}

double
IsaRadiansFromDegrees(double Degrees)
{
    double Radians = 0.01745329251994329577f * Degrees;
    return Radians;
}

////////////////////////////////////////
//            ALLOCATORS              //
////////////////////////////////////////

void
IsaMemZero(void *Mem, size_t Size)
{
    for(size_t i = 0; i < Size; ++i)
    {
        ((u8 *)Mem)[i] = 0;
    }
}

#define IsaMemZeroStruct(struct) IsaMemZero(struct, sizeof(*struct))

typedef struct isa_arena
{
    u8    *Mem;
    size_t Top;
    size_t Size;
} isa_arena;

isa_arena
IsaArenaCreate(void *Mem, size_t Size)
{
    isa_arena Arena;
    Arena.Mem  = (u8 *)Mem;
    Arena.Top  = 0;
    Arena.Size = Size;

    return Arena;
}

void
IsaArenaDestroy(isa_arena **Arena)
{
    if(Arena && *Arena)
    {
        *Arena = NULL;
        Arena  = NULL;
    }
}

void *
IsaArenaPush(isa_arena *Arena, size_t Size)
{
    u8 *AllocedMem = Arena->Mem + Arena->Top;
    Arena->Top += Size;

    return (void *)AllocedMem;
}

void *
IsaArenaPushZero(isa_arena *Arena, size_t Size)
{
    u8 *AllocedMem = Arena->Mem + Arena->Top;
    IsaMemZero(AllocedMem, Size);
    Arena->Top += Size;

    return (void *)AllocedMem;
}

void
IsaArenaPop(isa_arena *Arena, size_t Size)
{
    assert(Arena->Top >= Size);
    Arena->Top -= Size;
}

size_t
IsaArenaGetPos(isa_arena *Arena)
{
    size_t Result = Arena->Top;
    return Result;
}

void
IsaArenaSeek(isa_arena *Arena, size_t Pos)
{
    assert(0 <= Pos && Pos <= Arena->Size);
    Arena->Top = Pos;
}

void
IsaArenaClear(isa_arena *Arena)
{
    Arena->Top = 0;
}

#define IsaPushArray(arena, type, count) (type *)IsaArenaPush((arena), sizeof(type) * (count))
#define IsaPushArrayZero(arena, type, count) (type *)IsaArenaPushZero((arena), sizeof(type) * (count))

#define IsaPushStruct(arena, type) PushArray((arena), (type), 1)
#define IsaPushStructZero(arena, type) IsaPushArrayZero((arena), (type), 1)

#define ISA_DEFINE_POOL_ALLOCATOR(type)                                                                                \
    typedef struct type##_Pool                                                                                         \
    {                                                                                                                  \
        isa_arena *Arena;                                                                                              \
        type      *FirstFree;                                                                                          \
    } type##_pool;                                                                                                     \
                                                                                                                       \
    type *type##Alloc(type##_pool *Pool)                                                                               \
    {                                                                                                                  \
        type *Result = Pool->FirstFree;                                                                                \
        if(Result)                                                                                                     \
        {                                                                                                              \
            Pool->FirstFree = Pool->FirstFree->Next;                                                                   \
            IsaMemZeroStruct(Result);                                                                                  \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            Result = IsaPushStructZero(Pool->Arena, type);                                                             \
        }                                                                                                              \
                                                                                                                       \
        return Result;                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    void type##Release(type##_pool *Pool, type *Instance)                                                              \
    {                                                                                                                  \
        Instance->Next  = Pool->FirstFree;                                                                             \
        Pool->FirstFree = Instance;                                                                                    \
    }

#define ISA_CREATE_POOL_ALLOCATOR(Name, type, Arena)                                                                   \
    type##_Pool Name;                                                                                                  \
    Name.Arena     = Arena;                                                                                            \
    Name.FirstFree = 0;

////////////////////////////////////////
//            MEM TRACE               //
////////////////////////////////////////

typedef struct
{
    bool  *Occupied;
    void **Pointer;
    char **Function;
    int   *Line;
    char **File;
} Isa__allocation_collection_entry;

typedef struct
{
    uint64_t End; // Final index in the arrays (Capacity - 1)
    bool     Initialized;
    uint64_t AllocationCount;

    bool  *Occupied;
    void **Pointer;
    char **Function;
    int   *Line;
    char **File;
} Isa__global_allocation_collection;

Isa__global_allocation_collection *
ISA__GetGlobalAllocationCollection()
{
    static Isa__global_allocation_collection Collection = { 0 };
    return &Collection;
}

// NOTE(ingar): This will work even if the struct has never had its members
// allocated before
//  but the memory will not be zeroed the first time around, so we might want to
//  do something about that
bool
ISA__AllocGlobalPointerCollection(uint64_t NewCapacity)
{
    Isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();
    uint64_t                           NewEnd     = NewCapacity - 1;
    if(Collection->End >= NewEnd)
    {
        // TODO(ingar): Error handling
        return false;
    }

    Collection->End = NewEnd;

    void *OccupiedRealloc = realloc(Collection->Occupied, NewCapacity);
    void *PointerRealloc  = realloc(Collection->Pointer, NewCapacity);
    void *FunctionRealloc = realloc(Collection->Function, NewCapacity);
    void *LineRealloc     = realloc(Collection->Line, NewCapacity);
    void *FileRealloc     = realloc(Collection->File, NewCapacity);

    if(!OccupiedRealloc || !PointerRealloc || !FunctionRealloc || !LineRealloc || !FileRealloc)
    {
        // TODO(ingar): Error handling
        return false;
    }

    Collection->Occupied = (bool *)OccupiedRealloc;
    Collection->Pointer  = (void **)PointerRealloc;
    Collection->Function = (char **)FunctionRealloc;
    Collection->Line     = (int *)LineRealloc;
    Collection->File     = (char **)FileRealloc;

    return true;
}

Isa__allocation_collection_entry
ISA__GetGlobalAllocationCollectionEntry(void *Pointer)
{
    Isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();

    uint64_t Idx = 0;
    for(; Idx <= Collection->End; ++Idx)
    {
        if(Collection->Pointer[Idx] == Pointer)
        {
            break;
        }
    }

    if(Idx > Collection->End)
    {
        // TODO(ingar): Error handling
        Isa__allocation_collection_entry Entry = { 0 };
        return Entry;
    }

    Isa__allocation_collection_entry Entry;

    Entry.Occupied = &Collection->Occupied[Idx];
    Entry.Pointer  = &Collection->Pointer[Idx];
    Entry.Function = &Collection->Function[Idx];
    Entry.Line     = &Collection->Line[Idx];
    Entry.File     = &Collection->File[Idx];

    return Entry;
}

void
ISA__RegisterNewAllocation(void *Pointer, const char *Function, int Line, const char *File)
{
    Isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();

    // TODO(ingar): This loop should never fail if we don't run out of memory
    //  but I should still add some error handling at some point
    uint64_t EntryIdx = 0;
    for(uint64_t i = 0; i <= Collection->End; ++i)
    {
        if(i > Collection->End)
        {
            uint64_t NewCapacity = (uint64_t)(1.5 * (double)Collection->End);
            if(NewCapacity <= Collection->End)
            {
                // TODO(ingar): Handle wrapping
            }
            ISA__AllocGlobalPointerCollection(NewCapacity);
        }

        if(!Collection->Occupied[i])
        {
            EntryIdx = i;
            break;
        }
    }

    size_t FunctionNameLength = strlen(Function) + 1;
    size_t FileNameLength     = strlen(File) + 1;

    char *FunctionNameString = (char *)malloc(FunctionNameLength);
    char *FileNameString     = (char *)malloc(FileNameLength);

    if(!FunctionNameString || !FileNameString)
    {
        // TODO(ingar): Error handling
    }

    strcpy(FunctionNameString, Function);
    strcpy(FileNameString, File);

    Collection->Occupied[EntryIdx] = true;
    Collection->Pointer[EntryIdx]  = Pointer;
    Collection->Function[EntryIdx] = FunctionNameString;
    Collection->Line[EntryIdx]     = Line;
    Collection->File[EntryIdx]     = FileNameString;

    Collection->AllocationCount++;
}

/**
 * @note Assumes that Pointer is not null
 */
void
ISA__RemoveAllocationFromGlobalCollection(void *Pointer)
{
    Isa__allocation_collection_entry Entry = ISA__GetGlobalAllocationCollectionEntry(Pointer);
    if(!Entry.Pointer)
    {
        // TODO(ingar): Error handling
    }

    *Entry.Occupied = false;
    *Entry.Line     = 0;
    free(*Entry.Function);
    free(*Entry.File);

    ISA__GetGlobalAllocationCollection()->AllocationCount--;
}

void
ISA__UpdateRegisteredAllocation(void *Original, void *New)
{
    Isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();

    uint64_t Idx = 0;
    for(; Idx <= Collection->End; ++Idx)
    {
        if(Collection->Pointer[Idx] == Original)
        {
            break;
        }
    }

    if(Idx > Collection->End)
    {
        // TODO(ingar): Error handling
        return;
    }

    Collection->Pointer[Idx] = New;
}

void *
ISA__MallocTrace(size_t Size, const char *Function, int Line, const char *File)
{
    void *Pointer = malloc(Size);

    printf("MALLOC: In %s on line %d in %s:\n\n", Function, Line, File);
#if MEM_LOG
    ISA__RegisterNewAllocation(Pointer, Function, Line, File);
#endif

    return Pointer;
}

void *
ISA__CallocTrace(size_t ElementCount, size_t ElementSize, const char *Function, int Line, const char *File)
{
    void *Pointer = calloc(ElementCount, ElementSize);

    printf("CALLOC: In %s on line %d in %s\n\n", Function, Line, File);
#if MEM_LOG
    if(!Pointer)
    {
        return NULL;
    }
    ISA__RegisterNewAllocation(Pointer, Function, Line, File);
#endif

    return Pointer;
}

void *
ISA__ReallocTrace(void *Pointer, size_t Size, const char *Function, int Line, const char *File)
{
    if(!Pointer)
    {
        return NULL;
    }

    printf("REALLOC: In %s on line %d in %s\n", Function, Line, File);
#if MEM_LOG
    Isa__allocation_collection_entry Entry = ISA__GetGlobalAllocationCollectionEntry(Pointer);
    if(!Entry.Pointer)
    {
        // TODO(ingar): Error handling
    }
    printf("         Previously allocated in %s on line %d in %s\n\n", *Entry.Function, *Entry.Line, *Entry.File);
    ISA__RemoveAllocationFromGlobalCollection(Pointer);
#endif

    void *PointerRealloc = realloc(Pointer, Size);
    if(!PointerRealloc)
    {
        return NULL;
    }
    ISA__RegisterNewAllocation(PointerRealloc, Function, Line, File);

    return PointerRealloc;
}

bool
ISA__FreeTrace(void *Pointer, const char *Function, int Line, const char *File)
{
    if(!Pointer)
    {
        return false;
    }

    printf("FREE: In %s on line %d in %s:\n", Function, Line, File);
#if MEM_LOG
    Isa__allocation_collection_entry Entry = ISA__GetGlobalAllocationCollectionEntry(Pointer);
    if(!Entry.Pointer)
    {
        // TODO(ingar): Error handling
    }
    printf("      Allocated in %s on line %d in %s\n\n", *Entry.Function, *Entry.Line, *Entry.File);
    ISA__RemoveAllocationFromGlobalCollection(Pointer);
#endif

    free(Pointer);
    return true;
}

bool
IsaInitAllocationCollection(uint64_t Capacity)
{
    Isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();

    void *OccupiedRealloc = calloc(Capacity, sizeof(bool));
    void *PointerRealloc  = calloc(Capacity, sizeof(void *));
    void *FunctionRealloc = calloc(Capacity, sizeof(void *));
    void *LineRealloc     = calloc(Capacity, sizeof(void *));
    void *FileRealloc     = calloc(Capacity, sizeof(void *));

    if(!OccupiedRealloc || !PointerRealloc || !FunctionRealloc || !LineRealloc || !FileRealloc)
    {
        // TODO(ingar): Error handling
        return false;
    }

    Collection->Occupied = (bool *)OccupiedRealloc;
    Collection->Pointer  = (void **)PointerRealloc;
    Collection->Function = (char **)FunctionRealloc;
    Collection->Line     = (int *)LineRealloc;
    Collection->File     = (char **)FileRealloc;

    Collection->Initialized     = true;
    Collection->AllocationCount = 0;

    return true;
}

void
IsaPrintAllAllocations(void)
{
    Isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();
    if(Collection->AllocationCount > 0)
    {
        printf("DEBUG: Printing remaining allocations:\n");
        for(uint64_t i = 0; i <= Collection->End; ++i)
        {
            if(Collection->Occupied[i])
            {
                printf("\n\tIn %s on line %d in %s\n", Collection->Function[i], Collection->Line[i],
                       Collection->File[i]);
            }
        }
    }

    printf("\nDBEUG: There are %llu remaining allocations\n\n", Collection->AllocationCount);
}

////////////////////////////////////////
//               RANDOM               //
////////////////////////////////////////

uint32_t *
ISA__GetPCGState(void)
{
    static uint32_t ISA__PCGState = 0;
    return &ISA__PCGState;
}

// Implementation of the PCG algorithm (https://www.pcg-random.org)
// It's the caller's responsibilites to have called SeedRandPCG before use
uint32_t
IsaRandPCG(void)
{
    uint32_t State      = *ISA__GetPCGState();
    *ISA__GetPCGState() = State * 747796405u + 2891336453u;
    uint32_t Word       = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
    return (Word >> 22u) ^ Word;
}

void
IsaSeedRandPCG(uint32_t Seed)
{
    *ISA__GetPCGState() = Seed;
}

////////////////////////////////////////
//               ARRAY                //
////////////////////////////////////////

/////////////////////////////////////////
//              FILE IO                //
/////////////////////////////////////////

typedef struct
{
    size_t  Size;
    uint8_t Data[];
} Isa_file_data;

/**
 * @note Data is one byte longer than Size to include a null-terminator in case
 * we are working with strings. The null-terminator is always added since we use
 * calloc.
 */
Isa_file_data *
IsaLoadFileIntoMemory(const char *Filename)
{
    FILE *fd = fopen(Filename, "rb");
    if(!fd)
    {
        fprintf(stderr, "Could not open file!\n");
        return NULL;
    }

    if(fseek(fd, 0L, SEEK_END) != 0)
    {
        fprintf(stderr, "Could not seek file!\n");
        fclose(fd);
        return NULL;
    }

    size_t FileSize = (size_t)ftell(fd);
    rewind(fd);

    size_t         FileDataSize = sizeof(Isa_file_data) + FileSize + 1;
    Isa_file_data *FileData     = (Isa_file_data *)calloc(1, FileDataSize);
    if(!FileData)
    {
        fprintf(stderr, "Could not allocate memory for file!\n");
        fclose(fd);
        return NULL;
    }

    FileData->Size   = FileSize;
    size_t BytesRead = fread(FileData->Data, 1, FileSize, fd);
    if(BytesRead != FileSize)
    {
        fprintf(stderr, "Could not read file!\n");
        fclose(fd);
        free(FileData);
        return NULL;
    }

    fclose(fd);
    return FileData;
}

bool
IsaWriteBufferToFile(void *Buffer, size_t ElementSize, uint64_t ElementCount, const char *Filename)
{
    FILE *fd = fopen(Filename, "wb");
    if(!fd)
    {
        fprintf(stderr, "Unable to open file %s!\n", Filename);
        return false;
    }

    bool WriteSuccessful = fwrite(Buffer, ElementSize, ElementCount, fd) == ElementCount;
    fclose(fd);
    return WriteSuccessful;
}

bool
IsaWrite_file_data_ToFile(Isa_file_data *FileData, const char *Filename)
{
    FILE *fd = fopen(Filename, "wb");
    if(!fd)
    {
        fprintf(stderr, "Failed to open file during file_data write!\n");
        return false;
    }

    bool WriteSuccessful = fwrite(FileData->Data, sizeof(uint8_t), FileData->Size, fd) == FileData->Size;
    fclose(fd);
    return WriteSuccessful;
}

////////////////////////////////////////
//             TOKENIZER              //
////////////////////////////////////////

// TODO(ingar): This is not a general purpose tokenizer,
// but it is an example of an implementation
typedef struct isa_token
{
    char  *Start;
    size_t Len;
} isa_token;

isa_token
IsaGetNextToken(char **Cursor)
{
    while('\t' != **Cursor)
    {
        (*Cursor)++;
    }

    (*Cursor)++; // Skips to start of hex number

    isa_token Token;
    Token.Start = *Cursor;
    Token.Len   = 0;

    while('\n' != **Cursor && '\r' != **Cursor)
    {
        (*Cursor)++;
        ++Token.Len;
    }

    if('\0' != **Cursor)
    {
        **Cursor = '\0';
    }

    return Token;
}

////////////////////////////////////////
//              LOGGING               //
////////////////////////////////////////

/*
    TOOO(ingar): Make functions for the logging part and change the macros to
   call those.
*/

#if !defined(NDEBUG)

#define OEC_LOG_LEVEL_NONE (0U)
#define OEC_LOG_LEVEL_ERR (1U)
#define OEC_LOG_LEVEL_WRN (2U)
#define OEC_LOG_LEVEL_INF (3U)
#define OEC_LOG_LEVEL_DBG (4U)

#define OEC_LOG_LEVEL_CHECK(level) (OEC_LOG_LEVEL >= OEC_LOG_LEVEL_##level ? 1 : 0)

struct oec_log_module_
{
    char               buf[OEC_LOG_BUF_SIZE];
    size_t             buf_size;
    struct oec_stream *stream;
    const char        *name;
};

/**
 * Redirects to printf
 */
ssize_t oec_log_default_stream_write__(struct oec_stream *, void *, size_t);

#define OEC_LOG_REGISTER(module_name)                                                                                  \
    static struct oec_stream oec_log_default_stream__ = {                                                              \
        .data  = NULL,                                                                                                 \
        .read  = NULL,                                                                                                 \
        .write = oec_log_default_stream_write__,                                                                       \
    };                                                                                                                 \
                                                                                                                       \
    struct oec_log_module_ OEC_CONCAT3(oec_log_instance_, module_name, __) __attribute__((used))                       \
    = { .buf = { 0 }, .buf_size = OEC_LOG_BUF_SIZE, .stream = &oec_log_default_stream__, .name = #module_name };       \
                                                                                                                       \
    static struct oec_log_module_ *oec_log_instance__ = &OEC_CONCAT3(oec_log_instance_, module_name, __)

#define OEC_LOG_DECLARE(name)                                                                                          \
    extern struct oec_log_module_  OEC_CONCAT3(oec_log_instance_, name, __);                                           \
    static struct oec_log_module_ *oec_log_instance__ = &OEC_CONCAT3(oec_log_instance_, name, __)

#define OEC_LOG_SET_STREAM(new_stream)                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        oec_log_instance__->stream = new_stream;                                                                       \
    } while(0)

int oec_write_log_(struct oec_log_module_ *, const char *, ...);
#define OEC_LOG__(log_level, ...)                                                                                      \
    do                                                                                                                 \
    {                                                                                                                  \
        if(OEC_LOG_LEVEL_CHECK(log_level))                                                                             \
        {                                                                                                              \
            int ret = oec_write_log_(oec_log_instance__, #log_level, __VA_ARGS__);                                     \
            assert(0 == ret);                                                                                          \
        }                                                                                                              \
    } while(0)

#define OEC_LOG_DBG(...) OEC_LOG__(DBG, __VA_ARGS__)

#define OEC_LOG_INF(...) OEC_LOG__(INF, __VA_ARGS__)

#define OEC_LOG_WRN(...) OEC_LOG__(WRN, __VA_ARGS__)

#define OEC_LOG_ERR(...) OEC_LOG__(ERR, __VA_ARGS__)

OEC_END_DECL__

int
oec_write_log_(struct oec_log_module_ *module, const char *log_level, ...)
{
    time_t    posix_time;
    struct tm time_info;

    (void)time(&posix_time);
    (void)localtime_r(&posix_time, &time_info);

    size_t buf_remaining = module->buf_size;
    size_t chars_written = strftime(module->buf, module->buf_size, "%T: ", &time_info);

    if(0 == chars_written)
    {
        return -1;
    }

    buf_remaining -= chars_written;

    int ret = snprintf(module->buf + chars_written, buf_remaining, "%s: %s: ", module->name, log_level);
    if((ret < 0) || ((size_t)ret >= buf_remaining))
    {
        return -1;
    }

    chars_written += ret;
    buf_remaining -= ret;

    va_list args;
    va_start(args, log_level);

    const char *fmt;
    fmt = va_arg(args, const char *);

    chars_written += vsnprintf(module->buf + chars_written, buf_remaining, fmt, args);

    va_end(args);

    if(chars_written > module->buf_size)
    {
        return -1;
    }

    oec_errno err = oec_stream_write(module->stream, module->buf, chars_written);

    return err < 0 ? -1 : 0;
}
#endif // NDEBUG

////////////////////////////////////////
//               MACROS               //
////////////////////////////////////////

// NOTE(ingar): There might be trouble with the preprocessor replacing
//               malloc, calloc, etc. inside this file.
#if MEM_TRACE
#define malloc(Size) ISA__MallocTrace(Size, __func__, __LINE__, __FILE__)
#define calloc(Count, Size) ISA__CallocTrace(Count, Size, __func__, __LINE__, __FILE__)
#define realloc(Pointer, Size) ISA__ReallocTrace(Pointer, Size, __func__, __LINE__, __FILE__)
#define free(Pointer) ISA__FreeTrace(Pointer, __func__, __LINE__, __FILE__)

#else // MEM_TRACE

#define malloc(Size) malloc(Size)
#define calloc(Count, Size) calloc(Count, Size)
#define realloc(Pointer, Size) realloc(Pointer, Size)
#define free(Pointer) free(Pointer)
#endif // MEM_TRACE

#if 0 // defined(_cplusplus)
}
#endif

#define ISA_H_
#endif
