#ifndef ISA_ALL_INCLUDE_HPP

#if (defined(_WIN32) || defined(_WIN64)) && !defined( _CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
/*
    TODO(ingar): Add error handling for initialization stuff
    TODO(ingar): Split into C and C++ sections to pound-if out C++ stuff in C-mode? 
*/

#include <math.h>
#include <chrono>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> //NOTE(ingar): Replace with our own functions?
#include <assert.h>
#include <iostream>

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

#if !defined(_cplusplus)
#include <stdbool.h>
#endif

#define isa_internal static
#define isa_persist static
#define isa_global static
////////////////////////////////////////
//                MISC                //                
////////////////////////////////////////

#define KiloByte(Number) (Number * 1024ULL)
#define MegaByte(Number) (KiloByte(Number) * 1024ULL)
#define GigaByte(Number) (MegaByte(Number) * 1024ULL)
#define TeraByte(Number) (GigaByte(Number) * 1024ULL)

bool
IsaDoubleEpsilonCompare(const double A, const double B)
{
    double GreatestValue = (fabs(A) < fabs(B)) ? fabs(B) : fabs(A);
    return fabs(A - B) <= (GreatestValue * DBL_EPSILON);
}

uint64_t
IsaDoubleSignBit(double F)
{
    uint64_t Mask  = 1ULL << 63;
    uint64_t *Comp = (uint64_t *)&F;
    
    return (*Comp) & Mask;
}

double
IsaRadiansFromDegrees(double Degrees)
{
    double Radians = 0.01745329251994329577f * Degrees;
    return Radians;
}

template <typename T>
int
IsaCompare(const T A, const T B)
{
    return (A > B) - (A < B);
}

template <typename T>
void
IsaSwap(T &a, T &b) {
    T temp = a;
    a = b;
    b = temp;
}

////////////////////////////////////////
//                ALLOC               //                
////////////////////////////////////////

typedef struct
{
    bool    *Occupied;
    void   **Pointer;
    char   **Function;
    int     *Line;
    char   **File;
}
isa__allocation_collection_entry;

typedef struct
{
    uint64_t End; //Final index in the arrays (Capacity - 1)
    bool     Initialized;
    uint64_t AllocationCount;

    bool    *Occupied;
    void   **Pointer;
    char   **Function;
    int     *Line;
    char   **File;
}
isa__global_allocation_collection;

isa__global_allocation_collection *
ISA__GetGlobalAllocationCollection()
{
    static isa__global_allocation_collection Collection = {0};
    return &Collection;
}

//NOTE(ingar): This will work even if the struct has never had its members allocated before
// but the memory will not be zeroed the first time around, so we might want to do something about that
bool
ISA__AllocGlobalPointerCollection(uint64_t NewCapacity)
{
    isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();
    uint64_t NewEnd = NewCapacity - 1;
    if(Collection->End >= NewEnd)
    {
        // TODO(ingar): Error handling
        return false;
    }

    Collection->End = NewEnd;

    void *OccupiedRealloc = realloc(Collection->Occupied, NewCapacity);
    void *PointerRealloc  = realloc(Collection->Pointer,  NewCapacity);
    void *FunctionRealloc = realloc(Collection->Function, NewCapacity);
    void *LineRealloc     = realloc(Collection->Line,     NewCapacity);
    void *FileRealloc     = realloc(Collection->File,     NewCapacity);

    if(!OccupiedRealloc || !PointerRealloc || 
       !FunctionRealloc || !LineRealloc    || !FileRealloc)
    {
        //TODO(ingar): Error handling
        return false;
    }

    Collection->Occupied = (bool  *) OccupiedRealloc;
    Collection->Pointer  = (void **) PointerRealloc;
    Collection->Function = (char **) FunctionRealloc;
    Collection->Line     = (int   *) LineRealloc;
    Collection->File     = (char **) FileRealloc;

    return true;
}

isa__allocation_collection_entry 
ISA__GetGlobalAllocationCollectionEntry(void *Pointer)
{
    isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();

    uint64_t Idx = 0;
    for(;Idx <= Collection->End; ++Idx)
    {
        if(Collection->Pointer[Idx] == Pointer) break;
    }

    if(Idx > Collection->End)
    {
        //TODO(ingar): Error handling
        isa__allocation_collection_entry Entry = {0};
        return Entry;
    }

    isa__allocation_collection_entry Entry;

    Entry.Occupied = &Collection->Occupied [Idx];
    Entry.Pointer  = &Collection->Pointer  [Idx];
    Entry.Function = &Collection->Function [Idx];
    Entry.Line     = &Collection->Line     [Idx];
    Entry.File     = &Collection->File     [Idx];

    return Entry;
}


void
ISA__RegisterNewAllocation(void *Pointer, const char *Function, int Line, const char *File)
{
    isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();
    
    //TODO(ingar): This loop should never fail if we don't run out of memory
    // but I should still add some error handling at some point
    uint64_t EntryIdx = 0;
    for(uint64_t i = 0; i <= Collection->End; ++i)
    {
        if(i > Collection->End)
        {
            uint64_t NewCapacity = (uint64_t)(1.5 * (double)Collection->End);
            if(NewCapacity <= Collection->End)
            {
                //TODO(ingar): Handle wrapping
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
    size_t FileNameLength     = strlen(File)     + 1;

    char *FunctionNameString  = (char *)malloc(FunctionNameLength);
    char *FileNameString      = (char *)malloc(FileNameLength);

    if(!FunctionNameString || !FileNameString)
    {
        //TODO(ingar): Error handling
    }

    strcpy(FunctionNameString, Function);
    strcpy(FileNameString,     File);

    Collection->Occupied [EntryIdx] = true;
    Collection->Pointer  [EntryIdx] = Pointer;
    Collection->Function [EntryIdx] = FunctionNameString;
    Collection->Line     [EntryIdx] = Line;
    Collection->File     [EntryIdx] = FileNameString;

    Collection->AllocationCount++;
}

/**
 * @note Assumes that Pointer is not null
*/
void
ISA__RemoveAllocationFromGlobalCollection(void *Pointer)
{
    isa__allocation_collection_entry Entry = ISA__GetGlobalAllocationCollectionEntry(Pointer);
    if(!Entry.Pointer)
    {
        //TODO(ingar): Error handling
    }

    *Entry.Occupied = false;
    *Entry.Line = 0;
    free(*Entry.Function);
    free(*Entry.File);

    ISA__GetGlobalAllocationCollection()->AllocationCount--;
}

void
ISA__UpdateRegisteredAllocation(void *Original, void *New)
{
    isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();

    uint64_t Idx = 0;
    for(;Idx <= Collection->End; ++Idx)
    {
        if(Collection->Pointer[Idx] == Original) break;
    }

    if(Idx > Collection->End)
    {
        //TODO(ingar): Error handling
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
    if(!Pointer) return NULL;
    ISA__RegisterNewAllocation(Pointer, Function, Line, File);
#endif

    return Pointer;
}

void *
ISA__ReallocTrace(void *Pointer, size_t Size, const char *Function, int Line, const char *File)
{
    if(!Pointer) return NULL;

    printf("REALLOC: In %s on line %d in %s\n", Function, Line, File);
#if MEM_LOG
    isa__allocation_collection_entry Entry = ISA__GetGlobalAllocationCollectionEntry(Pointer);
    if(!Entry.Pointer)
    {
        //TODO(ingar): Error handling
    }
    printf("         Previously allocated in %s on line %d in %s\n\n",
            *Entry.Function, *Entry.Line, *Entry.File);
    ISA__RemoveAllocationFromGlobalCollection(Pointer);
#endif

    void *PointerRealloc = realloc(Pointer, Size);
    if(!PointerRealloc) return NULL;
    ISA__RegisterNewAllocation(PointerRealloc, Function, Line,  File);

    return PointerRealloc;
}

bool
ISA__FreeTrace(void *Pointer, const char *Function, int Line, const char *File)
{
    if(!Pointer) return false;
    
    printf("FREE: In %s on line %d in %s:\n", Function, Line, File);
#if MEM_LOG
    isa__allocation_collection_entry Entry = ISA__GetGlobalAllocationCollectionEntry(Pointer);
    if(!Entry.Pointer)
    {
        //TODO(ingar): Error handling
    }
    printf("      Allocated in %s on line %d in %s\n\n", 
            *Entry.Function, *Entry.Line, *Entry.File);
    ISA__RemoveAllocationFromGlobalCollection(Pointer);
#endif

    free(Pointer);
    return true;
}

bool
IsaInitAllocationCollection(uint64_t Capacity)
{
    isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();

    void *OccupiedRealloc = calloc(Capacity, sizeof(bool));
    void *PointerRealloc  = calloc(Capacity, sizeof(void*));
    void *FunctionRealloc = calloc(Capacity, sizeof(void*));
    void *LineRealloc     = calloc(Capacity, sizeof(void*));
    void *FileRealloc     = calloc(Capacity, sizeof(void*));

    if(!OccupiedRealloc || !PointerRealloc || 
       !FunctionRealloc || !LineRealloc    || !FileRealloc)
    {
        //TODO(ingar): Error handling
        return false;
    }

    Collection->Occupied = (bool  *) OccupiedRealloc;
    Collection->Pointer  = (void **) PointerRealloc;
    Collection->Function = (char **) FunctionRealloc;
    Collection->Line     = (int   *) LineRealloc;
    Collection->File     = (char **) FileRealloc;

    Collection->Initialized     = true;
    Collection->AllocationCount = 0;

    return true;
}

void
IsaPrintAllAllocations(void)
{
    isa__global_allocation_collection *Collection = ISA__GetGlobalAllocationCollection();
    if(Collection->AllocationCount > 0)
    {
        printf("DEBUG: Printing remaining allocations:\n");
        for(uint64_t i = 0; i <= Collection->End; ++i)
        {
            if(Collection->Occupied[i])
            {
                printf("\n\tIn %s on line %d in %s\n", 
                        Collection->Function[i], Collection->Line[i], Collection->File[i]);
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
    uint32_t State = *ISA__GetPCGState();
    *ISA__GetPCGState() = State * 747796405u + 2891336453u;
    uint32_t Word = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
    return (Word >> 22u) ^ Word;
}

void
IsaSeedRandPCG(uint32_t Seed)
{
    *ISA__GetPCGState() = Seed;
}

template <typename T>
T
IsaRandomInRangePCG(T Min, T Max)
{
    double Scalar = (double)IsaRandPCG() / (double)(UINT32_MAX);
    return T(Min + (Scalar * (Max - Min)));
}

////////////////////////////////////////
//               ARRAY                //                
////////////////////////////////////////

#define IsaArraySize(Array) (sizeof(Array) / sizeof(Array[0]))

template <typename T>
void
IsaPrintArray(const T *Arr, uint64_t Len, uint64_t NewLinePos,
              const char *FormatString)
{
    for(uint64_t i = 0; i < Len; ++i)
    {
        if(i % NewLinePos == 0) printf("\n");
        printf(FormatString, Arr[i]);
    }
    printf("\n");
}

template <typename T>
bool
IsaArraysEqual(const T *A, const T *B, uint64_t Len)
{
    for(uint64_t i = 0; i < Len; ++i)
    {
        if(A[i] != B[i]) return false;
    }

    return true;
}

template <typename T>
T *
IsaRandomArrayPCG(uint64_t Len, T MinVal, T MaxVal)
{
    T *Arr = (T *)malloc(Len * sizeof(int32_t));
    if(!Arr) return 0;

    for(uint64_t i = 0; i < Len; ++i)
    {
        Arr[i] = IsaRandomInRangePCG<T>(MinVal, MaxVal);
    }

    return Arr;
}

template <typename T>
void
IsaFisherYatesShuffle(T *Arr, uint64_t Len)
{
    for(uint64_t i = Len - 1; i > 0; --i)
    {
        uint64_t j = IsaRandomInRangePCG<uint64_t>(0, i);
        Swap(Arr[i], Arr[j]);
    }
}


/////////////////////////////////////////
//              FILE IO                //                
/////////////////////////////////////////

typedef struct 
{
    size_t Size;
    uint8_t Data[];
}
isa_file_data;

/**
 * @note Data is one byte longer than Size to include a null-terminator in case we are working with strings.
 *       The null-terminator is always added since we use calloc.
*/
isa_file_data *
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

    size_t FileDataSize = sizeof(isa_file_data) + FileSize + 1;
    isa_file_data *FileData = (isa_file_data *)calloc(1, FileDataSize);
    if(!FileData)
    {
        fprintf(stderr, "Could not allocate memory for file!\n");
        fclose(fd);
        return NULL;
    }

    FileData->Size = FileSize;
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
IsaWriteBufferToFile(void *Buffer, size_t ElementSize,
                     uint64_t ElementCount, const char *Filename)
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
IsaWrite_file_data_ToFile(isa_file_data *FileData, const char *Filename)
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
//                QUEUE               //                
////////////////////////////////////////

template <typename T>
struct isa_queue
{
    uint64_t Front;
    uint64_t End;
    uint64_t Capacity;
    T Q[];
};

template <typename T>
isa_queue<T> *
IsaQConstruct(uint64_t Capacity)
{
    size_t QueueTotalSize = sizeof(isa_queue<T>) + (Capacity * sizeof(T));
    
    isa_queue<T> *Queue = (isa_queue<T> *)calloc(1, QueueTotalSize);
    if(!Queue) return 0;

    Queue->Capacity = Capacity;
    Queue->Front    = -1;
    Queue->End      = -1;

    return Queue;
}

template <typename T>
void
IsaQDestruct(isa_queue<T> *Queue)
{
    if(Queue) free(Queue);
}

template <typename T>
bool
IsaQIsEmpty(isa_queue<T> *Queue)
{
    return Queue->Front == -1;
}

template <typename T>
bool
IsaQIsFull(isa_queue<T> *Queue)
{
    return ((Queue->End + 1) % Queue->Capacity) == Queue->Front;
}

template <typename T>
bool
IsaQEnqueue(isa_queue<T> *Queue, uint32_t Val)
{
    if(IsaQIsFull(Queue)) return false;

    if(Queue->Front == -1) Queue->Front = 0;

    Queue->End           = (Queue->End + 1) % Queue->Capacity;
    Queue->Q[Queue->End] = Val;

    return true;
}

template <typename T>
T
IsaQDequeue(isa_queue<T> *Queue)
{
    if(IsaQIsEmpty(Queue)) return 0;

    T Item = Queue->Q[Queue->Front];
    if(Queue->Front == Queue->End) Queue->Front = Queue->End = -1;
    else                           Queue->Front = (Queue->Front + 1) % Queue->Capacity;

    return Item;
}

////////////////////////////////////////
//             BINARY HEAP            //                
////////////////////////////////////////

template <typename T>
using isa_binary_heap_comparator = bool(*)(const T&, const T&);

template <typename T>
bool
IsaMinHeapCompare(const T& Current, const T& Parent)
{
    return Current < Parent;
}

template <typename T>
bool
IsaMaxHeapCompare(const T& Current, const T& Parent)
{
    return Current > Parent;
}

template <typename T>
struct isa_binary_heap
{
    uint64_t Capacity;
    uint64_t Size;
    isa_binary_heap_comparator<T> Compare;
    T H[];
};

template <typename T>
isa_binary_heap<T> *
IsaBinaryHeapConstruct(uint64_t Capacity, isa_binary_heap_comparator<T> Comparator)
{
    size_t HeapTotalSize = sizeof(isa_binary_heap<T>) + (sizeof(T) * Capacity);
    isa_binary_heap<T> *Heap = (isa_binary_heap<T> *)calloc(1, HeapTotalSize);
    if(!Heap) return 0;

    Heap->Capacity = Capacity;
    Heap->Compare  = Comparator;
    return Heap;
}

template <typename T>
void
IsaBinaryHeapDestruct(isa_binary_heap<T> *Heap)
{
    if(Heap) free(Heap);
}

template <typename T>
isa_binary_heap<T> *
IsaBinaryHeapRealloc(uint64_t NewCapacity, isa_binary_heap<T> *Heap)
{
    if(NewCapacity < Heap->Size)
    {
        fprintf(stderr, "ERROR: Warning! Reallocating a heap with a smaller capacity"
                        "than the number of existing elements will cause loss of data!\n");
        return 0;
    }

    size_t NewHeapSize = sizeof(isa_binary_heap<T>) + (sizeof(T) * NewCapacity);
    isa_binary_heap<T> *NewHeap = (isa_binary_heap<T> *)realloc(Heap, NewHeapSize);
    if(!NewHeap) return 0;

    NewHeap->Capacity = NewCapacity;
    for(uint64_t i = Heap->Size; i < Heap->Capacity; ++i)
        Heap->H[i] = 0;
    
    return NewHeap;
}

template <typename T>
void 
IsaBinaryHeapHeapifyUp(isa_binary_heap<T> *Heap, uint64_t Index)
{
    uint64_t ParentIndex = (Index - 1) / 2;
    while((Index > 0) && Heap->Compare(Heap->H[Index], Heap->H[ParentIndex]))
    {
        Swap(Heap->H[Index], Heap->H[ParentIndex]);
        Index = ParentIndex;
        ParentIndex = (Index - 1) / 2;
    }
}

template <typename T>
void
IsaBinaryHeapHeapifyDown(isa_binary_heap<T> *Heap, uint64_t Index)
{
    uint64_t LeftChild = 2 * Index + 1;
    uint64_t RightChild = 2 * Index + 2;
    uint64_t LargestOrSmallest = Index;

    if((LeftChild < Heap->Size) && Heap->Compare(Heap->H[LeftChild], Heap->H[LargestOrSmallest]))
        LargestOrSmallest = LeftChild;

    if((RightChild < Heap->Size) && Heap->Compare(Heap->H[RightChild], Heap->H[LargestOrSmallest]))
        LargestOrSmallest = RightChild;

    if(LargestOrSmallest != Index)
    {
        Swap(Heap->H[Index], Heap->H[LargestOrSmallest]);
        IsaBinaryHeapHeapifyDown(Heap, LargestOrSmallest);
    }
}

template <typename T>
void
IsaBinaryHeapHeapify(isa_binary_heap<T> *Heap, uint64_t Index)
{
    uint64_t ParentIndex = (Index - 1) / 2;
    if(Index > 0 && Heap->Compare(Heap->H[Index], Heap->H[ParentIndex]))
    {
        BinaryHeapHeapifyUp(Heap, Index);
    }
    else
    {
        IsaisaBinaryHeapHeapifyDown(Heap, Index);
    }
}

template <typename T>
void
IsaBinaryHeapInsert(isa_binary_heap<T> *Heap, T Value)
{
    if(Heap->Size >= Heap->Capacity)
    {
        fprintf(stderr, "ERROR: Heap overflow!\n");
        return;
    }

    Heap->H[Heap->Size] = Value;
    Heap->Size++;
    IsaBinaryHeapHeapifyUp(Heap, Heap->Size - 1);
}

template <typename T>
T
IsaBinaryHeapPop(isa_binary_heap<T> *Heap)
{
    if(Heap->Size == 0)
    {
        fprintf(stderr, "ERROR: Empty heap!\n");
        return {0};
    }

    T MinValue = Heap->H[0];
    Heap->H[0] = Heap->H[Heap->Size - 1];
    Heap->Size--;
    IsaBinaryHeapHeapifyDown(Heap, 0);
    return MinValue;
}

template <typename T>
T
IsaBinaryHeapPeek(isa_binary_heap<T> *Heap)
{
    if(Heap->Size == 0)
    {
        fprintf(stderr, "ERROR: Empty heap!\n");
        return 0;
    }

    T Root = Heap->H[0];
    return Root;
}

template <typename T>
void
IsaBinaryHeapIncreaseKey(isa_binary_heap<T> *Heap, uint64_t Index, T NewValue)
{
    // Won't work on compound types 
    if(NewValue < Heap->H[Index])
    {
        fprintf(stderr, "ERROR: New key is smaller than the current key!\n");
        return;
    }

    Heap->H[Index] = NewValue;
    IsaBinaryHeapHeapifyUp(Heap, Index);
}

template <typename T>
void
IsaBinaryHeapDecreaseKey(isa_binary_heap<T> *Heap, uint64_t Index, T NewValue)
{
    // Won't work on compound types
    if(NewValue > Heap->H[Index])
    {
        fprintf(stderr, "ERROR: New key is larger than the current key at!\n");
        return;
    }

    Heap->H[Index] = NewValue;
    IsaBinaryHeapHeapifyDown(Heap, Index);
}

template <typename T>
void
IsaBinaryHeapBuild(isa_binary_heap<T> *Heap, T *Array, uint64_t ArraySize)
{
    if(ArraySize > Heap->Capacity)
    {
        fprintf(stderr, "ERROR: Array size exceeds heap capacity!\n");
        return;
    }

    memcpy(Heap->H, Array, sizeof(T) * ArraySize);
    Heap->Size = ArraySize;
    for(int64_t i = (ArraySize / 2) - 1; i >= 0; --i)
    {
        IsaBinaryHeapHeapifyDown(Heap, i);
    }
}


////////////////////////////////////////
//           SCALAR QUICKSORT         //                
////////////////////////////////////////

template <typename T>
T
IsaMedian3Sort(T *Arr, uint64_t L, uint64_t R)
{
    uint64_t Mid = (L + R) / 2;

    if(Arr[L] > Arr[Mid])
    {
        Swap(Arr[L], Arr[Mid]);
    }

    if(Arr[Mid] > Arr[R])
    {
        Swap(Arr[Mid], Arr[R]);

        if(Arr[L] > Arr[Mid])
        {
            Swap(Arr[L], Arr[Mid]);
        }
    }

    return Mid;
}

template <typename T>
T
IsaSplit(T *Arr, uint64_t L, uint64_t R)
{
    uint64_t Mid = Median3Sort(Arr, L, R);
    T SplitVal = Arr[Mid];

    Swap(Arr[Mid], Arr[R - 1]);

    uint64_t iL, iR;
    for(iL = L, iR = R - 1;;)
    {
        while(Arr[++iL] < SplitVal);
        while(Arr[--iR] > SplitVal);

        if(iL >= iR) break;

        Swap(Arr[iL], Arr[iR]);
    }

    Swap(Arr[iL], Arr[R - 1]);

    return iL;
}

template <typename T>
void
IsaFindMinAndMaxPos(T *Arr, uint64_t Len,
                    uint64_t *MinPos, uint64_t *MaxPos)
{
    T Min = Arr[0];
    T Max = Arr[0];
    uint64_t MinIdx = 0;
    uint64_t MaxIdx = 0;

    for(uint64_t i = 0; i < Len; ++i)
    {
        T Num = Arr[i];

        if(Num < Min)
        {
            Min = Num;
            MinIdx = i;
        }

        if(Num > Max)
        {
            Max = Num;
            MaxIdx = i;
        }
    }

    *MinPos = MinIdx;
    *MaxPos = MaxIdx;
}

template <typename T>
void
IsaQuicksortRecursiveStep(T *Arr, uint64_t ArrayLen, uint64_t L, uint64_t R)
{
    if(Arr[L - 1] == Arr[R + 1]) return;

    if(R - L <= 2)
    {
        Median3Sort(Arr, L, R);
        return;
    }

    uint64_t SplitPos = Split(Arr, L, R);
    QuicksortRecursiveStep(Arr, ArrayLen, L, SplitPos - 1);
    QuicksortRecursiveStep(Arr, ArrayLen, SplitPos + 1, R);

}

template <typename T>
void
IsaQuicksort(T *Arr, uint64_t ArrayLen)
{
    uint64_t MinPos, MaxPos;
    FindMinAndMaxPos(Arr, ArrayLen, &MinPos, &MaxPos);

    Swap(Arr[0], Arr[MinPos]);
    Swap(Arr[ArrayLen - 1], Arr[MaxPos]);

    QuicksortRecursiveStep(Arr, ArrayLen, 1, ArrayLen - 2);
}

////////////////////////////////////////
//              TIMING                //                
////////////////////////////////////////


typedef std::chrono::high_resolution_clock::time_point time_point;
typedef std::chrono::duration<double> time_duration;

void
PrintErrTimingDisabled(void)
{
    printf("WARNING: Timing has not been enabled! "
           "Define DO_TIMING before the header to enable it.\n");
}

struct timing_data
{
    uint32_t NthTiming;
    time_point StartTime;
    time_point EndTime;
    time_duration Duration;
};

struct timing
{
    time_point StartTime;
    time_point EndTime;
};

struct timings
{
    bool TimingStarted;
    uint32_t NTimings;
    uint32_t idx;

    time_point InitTime;
    timing *Timings;
};


/*
    Private functions. Should only be called through provided macros.
*/
static timings *
GetTimings__()
{
    static timings Timings = { false, 0, 0, time_point(), NULL };
    return &Timings;
}

inline void
StartTiming__(time_point Time)
{
    timings *Timings = GetTimings__();
    assert(!Timings->TimingStarted);
    
    Timings->idx += 1;
    Timings->Timings[Timings->idx].StartTime = Time;
    Timings->TimingStarted = true;
}

inline void
EndTiming__(time_point Time)
{
    timings *Timings = GetTimings__();
    assert(Timings->TimingStarted);

    Timings->Timings[Timings->idx].EndTime = Time;
    Timings->TimingStarted = false;
}


/*
    We use macros so the timing is not done inside a function call
*/
#ifdef DO_TIMING
#define START_TIMING() \
do { \
    time_point TimeStart = std::chrono::high_resolution_clock::now(); \
    StartTiming__(TimeStart); \
} while(0)

#define END_TIMING() \
do { \
    time_point TimeEnd = std::chrono::high_resolution_clock::now(); \
    EndTiming__(TimeEnd); \
} while(0)
#else
#define START_TIMING()
#define END_TIMING()
#endif


/*
    Public functions.
*/
timing_data
GetTimingData(uint32_t NthTiming)
{
#ifndef DO_TIMING
//    PrintErrTimingDisabled();
    return {0};
#else
    timings *Timings = GetTimings__();
    if(NthTiming < 1 || NthTiming >= Timings->NTimings) { return {0}; }

    uint32_t idx  = NthTiming - 1;
    timing Timing = Timings->Timings[idx];

    time_point    StartTime = Timing.StartTime;
    time_point    EndTime   = Timing.EndTime;
    time_duration Duration  = std::chrono::duration_cast<time_duration>(EndTime - StartTime);

    timing_data TimingData = 
    {
        NthTiming,
        StartTime,
        EndTime,
        Duration
    };

    return TimingData;
#endif
}

timing_data
GetLastTimingData(void)
{
#ifndef DO_TIMING
//    PrintErrTimingDisabled();
    return {0};
#else
    return GetTimingData(GetTimings__()->idx + 1);
#endif
}

void
PrintTimingData(timing_data TimingData)
{
#ifndef DO_TIMING
//    PrintErrTimingDisabled();
    return;
#else
    timings *Timings = GetTimings__();

    std::cout << "Timing nr. " << TimingData.NthTiming << ":\n";

    time_duration StartTimeSinceInit = std::chrono::duration_cast<time_duration>(TimingData.StartTime - Timings->InitTime);
    std::cout << "Start time since init: " << StartTimeSinceInit.count() << " seconds\n";

    time_duration EndTimeSinceInit = std::chrono::duration_cast<time_duration>(TimingData.EndTime - Timings->InitTime);
    std::cout << "End time since init: " << EndTimeSinceInit.count() << " seconds\n";

    std::cout << "Timing duration: " << TimingData.Duration.count() << " seconds\n";
#endif
}

void
PrintTimingData(uint32_t NthTiming)
{
#ifndef DO_TIMING
//    PrintErrTimingDisabled();
    return;
#else
    timing_data TimingData = GetTimingData(NthTiming);
    PrintTimingData(TimingData);
#endif
}

void
PrintTimingData(uint32_t NthTiming, const char *Message)
{
#ifndef DO_TIMING
//    PrintErrTimingDisabled();
    return;
#else
    printf(Message);
    timing_data TimingData = GetTimingData(NthTiming);
    PrintTimingData(TimingData);
#endif
}

void
PrintLastTimingData(void)
{
#ifndef DO_TIMING
//    PrintErrTimingDisabled();
    return;
#else
    timing_data TimingData = GetTimingData(GetTimings__()->idx + 1);
    PrintTimingData(TimingData);
#endif
}

int
InitTiming(uint32_t NTimings)
{
#ifndef DO_TIMING
//    PrintErrTimingDisabled();
    return 1;
#else
    timings *Timings = GetTimings__();

    Timings->NTimings = NTimings;
    Timings->idx = -1; // Will be incremented to 0 on first timing
    Timings->Timings = (timing *)calloc(NTimings, sizeof(timing)); 

    if(NULL == Timings->Timings)
    {
        return 1;
    }

    Timings->InitTime = std::chrono::high_resolution_clock::now();

    return 0;
#endif
}


////////////////////////////////////////
//              LOGGING               //                
////////////////////////////////////////

/*
    TOOO(ingar): Make functions for the logging part and change the macros to call those.
*/

#if !defined(NDEBUG)

#define MEM_TRACE 1
#define MEM_LOG 1
#define ISA_ASSERT_TRACE 1
#define ISA_ASSERT_ON_LOG_ERROR 1

#define ISA_DO_LOG_INFO 1
#define ISA_DO_LOG_ERROR 1
#define ISA_DO_LOG_DEBUG 1

#define ISA_ALL_INFO_TRACE 0
#define ISA_ALL_ERROR_TRACE 0
#define ISA_ALL_DEBUG_TRACE 0

#define ISA_NO_INFO_TRACE 0
#define ISA_NO_ERROR_TRACE 0
#define ISA_NO_DEBUG_TRACE 0

#else //NDEBUG

#define MEM_TRACE 0
#define MEM_LOG 0
#define ISA_ASSERT_TRACE 0
#define ISA_ASSERT_ON_LOG_ERROR 0

#define ISA_DO_LOG_INFO 0
#define ISA_DO_LOG_ERROR 0
#define ISA_DO_LOG_DEBUG 0

#define ISA_ALL_INFO_TRACE 0
#define ISA_ALL_ERROR_TRACE 0
#define ISA_ALL_DEBUG_TRACE 0

#define ISA_NO_INFO_TRACE 0
#define ISA_NO_ERROR_TRACE 0
#define ISA_NO_DEBUG_TRACE 0

#endif //NDEBUG

bool
ISA__AssertTrace(bool Expression, const char *ExpressionString, int Line, const char *Function, const char *File)
{
    printf("ASSERTION: In %s on line %d in %s:\n\n" 
           "\t-> Assertion on expression \"%s\"\n", Function, Line, File, ExpressionString);
    assert(Expression);
    return true;
}

#if ISA_ASSERT_ON_LOG_ERROR
#define ISA_ErrAssert(Expression) assert(Expression)
#else
#define ISA_ErrAssert(Expression) ((void)0)
#endif

#if ISA_DO_LOG_INFO
#if ISA_ALL_INFO_TRACE
#define ISA_LOG_INFO(...) ISA_LOG_INFO_TRACE(__VA_ARGS__)
#else
#define ISA_LOG_INFO(...) \
    do { \
        printf("INFO: "); \
        printf( __VA_ARGS__); \
        printf("\n"); \
    } while (0)
#endif // ISA_ALL_INFO_TRACE

#if ISA_NO_INFO_TRACE
#define ISA_LOG_INFO_TRACE(...) ISA_LOG_INFO(__VA_ARGS__)
#else
#define ISA_LOG_INFO_TRACE(...) \
    do { \
        printf("INFO: In %s on line %d in %s:\n\n\t-> ", __func__, __LINE__, __FILE__); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } while (0)
#endif // ISA_NO_INFO_TRACE
#else
#define ISA_LOG_INFO(...) ((void)0)
#define ISA_LOG_INFO_TRACE(...) ((void)0)
#endif // ISA_DO_LOG_INFO

#if ISA_DO_LOG_ERROR
#if ISA_ALL_ERROR_TRACE
#define ISA_LOG_ERROR(...) ISA_LOG_ERROR_TRACE(__VA_ARGS__)
#else
#define ISA_LOG_ERROR(...) \
    do { \
        printf("ERROR: "); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        ISA_ErrAssert(false); \
    } while (0)
#endif // ISA_ALL_ERROR_TRACE

#if ISA_NO_ERROR_TRACE
#define ISA_LOG_ERROR_TRACE(...) ISA_LOG_ERROR(__VA_ARGS__)
#else
#define ISA_LOG_ERROR_TRACE(...) \
    do { \
        fprintf(stderr, "ERROR: In %s on line %d in %s:\n\n\t-> ", __func__, __LINE__, __FILE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        ISA_ErrAssert(false); \
    } while (0)
#endif // ISA_NO_ERROR_TRACE
#else
#define ISA_LOG_ERROR_TRACE(...) ((void)0)
#define ISA_LOG_ERROR(...) ((void)0)
#endif // ISA_DO_LOG_ERROR

#if ISA_DO_LOG_DEBUG
#if ISA_ALL_DEBUG_TRACE
#define ISA_LOG_DEBUG(...) ISA_LOG_DEBUG_TRACE(__VA_ARGS__)
#else
#define ISA_LOG_DEBUG(...) \
    do { \
        printf("DEBUG: "); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } while (0)
#endif

#if ISA_NO_DEBUG_TRACE
#define ISA_LOG_DEBUG_TRACE(...) ISA_LOG_DEBUG(__VA_ARGS__)
#else
#define ISA_LOG_DEBUG_TRACE(...) \
    do { \
        printf("DEBUG: In %s on line %d in %s:\n\n\t-> ", __func__, __LINE__, __FILE__); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    } while (0);
#endif // ISA_NO_DEBUG_TRACE
#else
#define ISA_LOG_DEBUG_TRACE(...) ((void)0)
#define ISA_LOG_DEBUG(...) ((void)0)
#endif // ISA_DO_LOG_DEBUG

#if ISA_ASSERT_TRACE
#define IsaAssert(Expression) ISA__AssertTrace(Expression, #Expression, __LINE__, __func__, __FILE__)
#else
#define IsaAssert(Expression) assert(Expression)
#endif


////////////////////////////////////////
//               MACROS               //                
////////////////////////////////////////

//NOTE(ingar): There might be trouble with the preprocessor replacing
//              malloc, calloc, etc. inside this file.
#if MEM_TRACE
#define malloc(Size)           ISA__MallocTrace(Size, __func__, __LINE__, __FILE__)
#define calloc(Count, Size)    ISA__CallocTrace(Count, Size, __func__, __LINE__, __FILE__)
#define realloc(Pointer, Size) ISA__ReallocTrace(Pointer, Size, __func__, __LINE__, __FILE__)
#define free(Pointer)          ISA__FreeTrace(Pointer, __func__, __LINE__, __FILE__)

#else //MEM_TRACE

#define malloc(Size)           malloc(Size)
#define calloc(Count, Size)    calloc(Count, Size)
#define realloc(Pointer, Size) realloc(Pointer, Size)
#define free(Pointer)          free(Pointer)
#endif//MEM_TRACE


#define ISA_ALL_INCLUDE_HPP
#endif
