#include "basics_memory.h"

ASSERT(DEFAULT_FACTOR                           , "the factor can't be 0");
ASSERT(DEFAULT_RESERVATION >= DEFAULT_COMMISSION, "the reservation should always be greater than the commission");

#include "basics_bits.h"

/******************************************************************************/
/* auxiliaries */

EXTERNAL void *CCALL memcpy (void *RESTRICT, const void *RESTRICT, unsigned long long);
EXTERNAL void *CCALL memmove(void *, const void *, unsigned long long);
EXTERNAL void *CCALL memset (void *, int, unsigned long long);

#define Copy(d, s, n) (void)memcpy(d, s, n)
#define Move(d, s, n) (void)memmove(d, s, n)
#define Fill(d, c, n) (void)memset(d, c, n)
#define Zero(d, n)    Fill(d, 0, n)

#define Maximum(a, b) ((a) >= (b) ? (a) : (b))

/******************************************************************************/

#if defined(SYSTEM_IS_WIN64)

EXTERNAL int __stdcall GetLastError(void);

EXTERNAL int __stdcall QueryPerformanceCounter  (unsigned long long *);
EXTERNAL int __stdcall QueryPerformanceFrequency(unsigned long long *);

EXTERNAL void __stdcall GetSystemInfo(void *);

EXTERNAL long long          __stdcall VirtualAlloc  (long long, unsigned long long, unsigned, unsigned);
EXTERNAL int                __stdcall VirtualFree   (long long, unsigned long long, unsigned);
EXTERNAL int                __stdcall VirtualProtect(long long, unsigned long long, unsigned, unsigned *);
EXTERNAL unsigned long long __stdcall VirtualQuery  (long long, void *, unsigned long long);

typedef union {
	Byte _size[64];
	struct {
		ALIGNAS(4) Byte _pad[4];
		unsigned int pageSize;
	};
} SystemInfo;

PRIVATE inline SystemInfo *my_GetSystemInfo(void) {
	PERSISTANT Boolean initialized = 0;
	PERSISTANT SystemInfo s;
	if (!initialized) {
		GetSystemInfo(&s);
		initialized = 1;
	}
	return &s;
}

Size QueryVirtualMemoryGranularity(void) {
	return my_GetSystemInfo()->pageSize;
}

#if 0 /* TODO(Emhyr): should we reserve by the allocation granularity, or remain by the commission granularity? */
Size QueryVirtualMemoryAllocationGranularity(void) {
	return my_GetSystemInfo()->allocationGranularity;
}
#endif

Address AllocateVirtualMemory(Size size) {
	Address result = VirtualAlloc(0, size, 0x00001000 | 0x00002000, 0x04);
	Assert(result, "");
	return result;
}

Address ReserveVirtualMemory(Size size) {
	Address result = VirtualAlloc(0, size, 0x00002000, 0x04);
	Assert(result, "");
	return result;
}

void ReleaseVirtualMemory(Address address, Size size) {
	int result = VirtualFree(address, 0, 0x00008000);
	Assert(result, "");
}

void CommitVirtualMemory(Address address, Size size) {
	long long result = VirtualAlloc(address, size, 0x00001000, 0x04);
	int e = GetLastError();
	Assert(result, "");
}

void DecommitVirtualMemory(Address address, Size size) {
	int result = VirtualFree(address, size, 0x00004000);
	Assert(result, "");
}

void ValidateVirtualMemory(Address address, Size size) {
	unsigned oldFlags;
	int result = VirtualProtect(address, size, 0x04, &oldFlags);
	Assert(result, "");
}

void InvalidateVirtualMemory(Address address, Size size) {
	unsigned oldFlags;
	int result = VirtualProtect(address, size, 0x01, &oldFlags);
	Assert(result, "");
}

Boolean CheckCommittedVirtualMemory(Address address, Size size) {
	struct {
		void              *BaseAddress;
		void              *AllocationBase;
		int                AllocationProtect;
		short              PartitionId;
		unsigned long long RegionSize;
		int                State;
		int                Protect;
		int                Type;
	} s;
	VirtualQuery(address, &s, sizeof(s));
	return !!(s.State & 0x00001000);

}

#endif


void TouchVirtualMemory(Address address, Size size) {
	Size granularity = QueryVirtualMemoryGranularity();
	for (Size i = 0; i < size; i += granularity)
		((volatile Byte *)address)[i] = ((Byte *)address)[i];
}

PRIVATE inline Address GetNextPage(Size *granularity, Address address) {
	*granularity = QueryVirtualMemoryGranularity();
	return AlignForwards(address, *granularity);
}

PRIVATE inline void DoNextPages(Boolean doAll, void (*procedure)(Address, Size), Address address, Address ending) {
	Size size;
	Address nextPage = GetNextPage(&size, address);
	if (nextPage < ending) {
		if (doAll) size *= (ending - nextPage) / size;
		procedure(nextPage, size);
	}
}

PRIVATE inline void DecommitNextPage(Address address, Address ending) {
	DoNextPages(0, DecommitVirtualMemory, address, ending);
}

#define DecommitNextPageWithContext(context)  DecommitNextPage((context)->address + (context)->extent, (context)->address + (context)->reservation)

PRIVATE inline void DecommitNextPages(Address address, Address ending) {
	DoNextPages(1, DecommitVirtualMemory, address, ending);
}

#define DecommitNextPagesWithContext(context) DecommitNextPages((context)->address + (context)->extent, (context)->address + (context)->reservation)

PRIVATE inline void InvalidateNextPage(Address address, Address ending) {
	DoNextPages(0, InvalidateVirtualMemory, address, ending);
}

#define InvalidateNextPageWithContext(context) InvalidateNextPage((context)->address + (context)->extent, (context)->address + (context)->commission)

PRIVATE inline void InvalidateNextPages(Address address, Address ending) {
	DoNextPages(1, InvalidateVirtualMemory, address, ending);
}

#define InvalidateNextPagesWithContext(context) InvalidateNextPages((context)->address + (context)->extent, (context)->address + (context)->commission)

/* validation *****************************************************************/

#define VALIDATECONTEXT1(context) do {                               \
	Assert((context)                                      , ""); \
	Assert((context)->reservation >= (context)->commission, ""); \
	Assert((context)->commission >= (context)->extent     , ""); \
} while (0)

#define VALIDATECONTEXT2(context) do {  \
	VALIDATECONTEXT1(context);      \
	Assert((context)->address, ""); \
	Assert((context)->factor,  ""); \
} while (0)

#define VALIDATECONTEXT3(context) do {                                                                                                                                                  \
	VALIDATECONTEXT2(context);                                                                                                                                                      \
	Assert(context->reservation && context->address && context->commission && context->extent, "uninitialized! attempted to deallocate despite the allocator being uninitialized"); \
} while (0)

#define VALIDATE1(size, alignment) do {        \
	Assert(size                     , ""); \
	Assert(CheckAlignment(alignment), ""); \
} while (0)

#define VALIDATE2(reservation, commission) do {                                                                \
	Assert((reservation) >= (commission), "the reservation should always be greater than the commission"); \
} while (0)

#define VALIDATE3(address, context) do {                                                                                     \
	Assert((address)                                                                                              , ""); \
	Assert((Address)(address) >= (context)->address && (Address)(address) < (context)->address + (context)->extent, ""); \
} while (0)

#define VALIDATERESULT1(result) do { \
	Assert(result, "overflowed! perhaps the allocator is misused, or the reservation is too small. if the reservation is superflouously large (e.g. 1GiB), it's likely the former case."); \
} while (0)

/* linear allocator ***********************************************************/

/* linear allocator / creation ************************************************/

void InitializeLinearAllocator(LinearAllocator *context) {
	if (!context->reservation) context->reservation = DEFAULT_RESERVATION;
	if (!context->address)     context->address     = ReserveVirtualMemory(context->reservation);
	if (!context->commission)  context->commission  = DEFAULT_COMMISSION;
	if (!context->factor)      context->factor      = DEFAULT_FACTOR;
	CommitVirtualMemory(context->address, context->commission);
}

LinearAllocator MakeLinearAllocator(Size reservation, Size commission, Size factor) {
	LinearAllocator result = {.reservation = reservation, .address = 0, .commission = commission, .factor = factor, .extent = 0};
	InitializeLinearAllocator(&result);
	return result;
}

void DebugInitializeLinearAllocator(LinearAllocator *context) {
	VALIDATECONTEXT1(context);
	InitializeLinearAllocator(context);
	VALIDATECONTEXT2(context);
}

LinearAllocator DebugMakeLinearAllocator(Size reservation, Size commission, Size factor) {
	VALIDATE2(reservation, commission);
	return MakeLinearAllocator(reservation, commission, factor);
}

/* linear allocator / destruction *********************************************/

void ClearLinearAllocator(LinearAllocator *context) {
	context->extent = 0;
}

PRIVATE inline void DoClearLinearAllocatorWaned(LinearAllocator *context) {
	if (context->commission) {
		DecommitVirtualMemory(context->address, context->commission);
		context->commission = 0;
	}
}

void ClearLinearAllocatorWaned(LinearAllocator *context) {
	ClearLinearAllocator(context);
	DoClearLinearAllocatorWaned(context);
}

void DebugClearLinearAllocator(LinearAllocator *context) {
	Assert(context->address && context->extent,   "attempted to deallocate despite nothing to deallocate. perhaps the allocator is uninitialized or unused?");
	Assert(context->commission > context->extent, "the commission should always be greater than the extent");
	ClearLinearAllocator(context);
}

void DebugClearLinearAllocatorWaned(LinearAllocator *context) {
	DebugClearLinearAllocator(context);
	DoClearLinearAllocatorWaned(context);
}

/* linear allocator / allocation **********************************************/

PRIVATE inline void *DoPush(Boolean *doZero, Size size, Size alignment, LinearAllocator *context) {
#if ENABLE_AUTOMATIC_INITIALIZATION
	if (!context->address) InitializeLinearAllocator(context);
#endif

	void *result;
	Size aligner = GaugeForwardAligner(context->address + context->extent, alignment);
	if (context->extent + aligner + size > context->commission) {
		Size pageSize = QueryVirtualMemoryGranularity();
		Size commission = AlignForwards(aligner + size, pageSize);
		if (size > pageSize / context->factor) commission *= context->factor;
		if (context->commission + commission <= context->reservation) {
			CommitVirtualMemory(context->address + context->commission, size);
			context->commission += commission;
			*doZero = 0;
		} else {
			result = 0;
			*doZero = 0;
			goto finished;
		}
	}

	context->extent += aligner;
	result = (void *)(context->address + context->extent);
	context->extent += size;

finished:
	return result;
}

void *Push(Size size, Size alignment, LinearAllocator *context) {
	Boolean doZero;
	return DoPush(&doZero, size, alignment, context);
}

void *PushZeroed(Size size, Size alignment, LinearAllocator *context) {
	Boolean doZero = 0;
	void *result = DoPush(&doZero, size, alignment, context);
	if (doZero) Zero(result, size);
	return result;
}

typedef struct {
	Size extent;
	Byte bytes[];
} FrameHeader;

PRIVATE inline FrameHeader *GetFrameHeader(Address address) {
	FrameHeader *header = (FrameHeader *)(long long)AlignBackwards(address - sizeof(FrameHeader), ALIGNOF(FrameHeader));
	return header;
}

void *PushFrame(Size size, Size alignment, LinearAllocator *context) {
#if ENABLE_AUTOMATIC_INITIALIZATION
	InitializeLinearAllocator(context);
#endif

	/* NOTE(Emhyr): we just need enough space for an aligned header before the aligned allocation */
	Size headerAligner = GaugeForwardAligner(context->address + context->extent, ALIGNOF(FrameHeader));
	Size headerSize = headerAligner + sizeof(FrameHeader);
	Size aligner = GaugeForwardAligner(headerSize, alignment);
	Size extent = context->extent;
	Size offset = headerSize + aligner;
	Byte *result = Push(offset + size, 1, context);
	result += offset;
	GetFrameHeader((Address)result)->extent = extent;
	return result;
}

void *PushFrameZeroed(Size size, Size alignment, LinearAllocator *context) {
	void *result = PushFrame(size, alignment, context);
	Zero(result, size);
	return result;
}

void *DebugPush(Size size, Size alignment, LinearAllocator *context) {
	return DebugPushZeroed(size, alignment, context);
}

void *DebugPushZeroed(Size size, Size alignment, LinearAllocator *context) {
	VALIDATE1(size, alignment);
	VALIDATECONTEXT1(context);
	void *result = PushZeroed(size, alignment, context);
	VALIDATECONTEXT2(context);
	VALIDATERESULT1(result);
	
	ValidateVirtualMemory((Address)result, size);
	InvalidateNextPages((Address)result + size, context->address + context->commission);
	return result;
}

void *DebugPushFrame(Size size, Size alignment, LinearAllocator *context) {
	return DebugPushFrameZeroed(size, alignment, context);
}

void *DebugPushFrameZeroed(Size size, Size alignment, LinearAllocator *context) {
	VALIDATE1(size, alignment);
	VALIDATECONTEXT1(context);
	void *result = PushFrameZeroed(size, alignment, context);
	VALIDATECONTEXT2(context);
	VALIDATERESULT1(result);
	
	ValidateVirtualMemory((Address)result, size);
	InvalidateNextPages((Address)result, context->address + context->extent);
	return result;
}

/* linear allocator / deallocation ********************************************/

PRIVATE inline Size GetPullExtent(Boolean *didUnderflow, Size size, Size alignment, LinearAllocator *context) {
	Size address = context->address + context->extent;
	Size newAddress = AlignBackwards(address - size, alignment);
	*didUnderflow = newAddress < address;
	return *didUnderflow ? 0 : newAddress - address;
}

void Pull(Size size, Size alignment, LinearAllocator *context) {
	Boolean didUnderflow;
	context->extent = GetPullExtent(&didUnderflow, size, alignment, context);
}

void PullWaned(Size size, Size alignment, LinearAllocator *context) {
	Pull(size, alignment, context);
	DecommitNextPageWithContext(context);
}

void PullFrame(void *address, LinearAllocator *context) {
	FrameHeader *header = GetFrameHeader((Address)address);
	context->extent = header->extent;
}

void PullFrameWaned(void *address, LinearAllocator *context) {
	PullFrame(address, context);
	DecommitNextPageWithContext(context);
}

PRIVATE inline void DoDebugPull(Size size, Size alignment, LinearAllocator *context) {
	VALIDATE1(size, alignment);
	VALIDATECONTEXT3(context);
	Boolean didUnderflow = 0;
	Size extent = GetPullExtent(&didUnderflow, size, alignment, context);
	Assert(!didUnderflow, "underflowed! if attempted to clear, use `ClearLinearAllocator`");
	context->extent = extent;
}

void DebugPull(Size size, Size alignment, LinearAllocator *context) {
	DoDebugPull(size, alignment, context);
}

void DebugPullWaned(Size size, Size alignment, LinearAllocator *context) {
	DoDebugPull(size, alignment, context);
	DecommitNextPagesWithContext(context);
}

void DebugPullFrame(void *address, LinearAllocator *context) {
	VALIDATE3(address, context);
	VALIDATECONTEXT3(context);
	PullFrame(address, context);
	InvalidateNextPagesWithContext(context);
}

void DebugPullFrameWaned(void *address, LinearAllocator *context) {
	DebugPullFrame(address, context);
	DecommitNextPagesWithContext(context);
}

/* granular allocator *********************************************************/

/*

the flags actually tails the allocator's memory in reverse word order. this is
to enable the ability to increase the amount of entries without moving any bytes,
and also allowed

"reverse word order" means that you can traverse the flags as follows:

	for (Size i = flagsCount - 1; i != 0; --i) {
		Bits64 flags = flagsArray[i];
		Size flagOffset = FindSetBit(flags);
		...
	}

for example:

E - entry
F - flags
# - empty

to expand this:
	[EEEEEEEEEEEEEEEEEEEEEEEE##############FFF]
transform it to this:
	[EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE#####FFFF]
*/

PRIVATE inline Size GaugeFlagsArraySize(Size quantity) {
	return quantity / WIDTHOF(Bits64) * sizeof(Bits64);
}

PRIVATE inline Bits64 *GetBeginningFlags(GranularAllocator *context) {
	return (Bits64 *)(context->address + context->reservation - sizeof(Bits64));
}

PRIVATE inline Bits64 *GetEndingFlags(GranularAllocator *context) {
	return (Bits64 *)((Address)GetBeginningFlags(context) - GaugeFlagsArraySize(context->quantity));
}

/* granular allocator / creation **********************************************/

void InitializeGranularAllocator(GranularAllocator *context) {
	if (!context->reservation) context->reservation = DEFAULT_RESERVATION;
	if (!context->address)     context->address     = ReserveVirtualMemory(context->reservation);
	if (!context->granularity) context->granularity = DEFAULT_GRANULARITY;
	if (!context->quantity)    context->quantity    = DEFAULT_QUANTITY;

	/* NOTE(Emhyr): we don't care if the granularity is an odd number here. should we? */
	/* NOTE(Emhyr): should we align the quantity to correspond with the amount of bytes committed for the flags? */

	Address ending = (Address)GetEndingFlags(context);
	CommitVirtualMemory(AlignBackwards(ending + sizeof(Bits64), QueryVirtualMemoryGranularity()), GaugeFlagsArraySize(context->quantity));
	
	/* NOTE(Emhyr): we assume that the reservation is enough for the blocks here. this is unsafe */
}

GranularAllocator CreateGranularAllocator(Size reservation, Size granularity, Size quantity) {
	GranularAllocator result = {.reservation = reservation, .granularity = granularity, .quantity = quantity};
	InitializeGranularAllocator(&result);
	return result;
}

/* granular allocator / destruction *******************************************/

void ClearGranularAllocator(GranularAllocator *context) {
	Assert(!"unimplemented");
}
 
void ClearGranularAllocatorWaned(GranularAllocator *context) {
	Assert(!"unimplemented");
}

/* granular allocator / allocation ********************************************/

void *Put(Size size, GranularAllocator *context) {
#if ENABLE_AUTOMATIC_INITIALIZATION
	if (!context->address) InitializeGranularAllocator(context);
#endif
	
	Size count = (size + context->granularity - 1) / context->granularity;
	Bits64 *beginning = GetBeginningFlags(context);
	BitLocation location = FindBits(count, beginning, GetEndingFlags(context), 1);
	if (!location.pointer) return 0;
	SetBits(count, location, 0, 1);
	Size index = (beginning - location.pointer) * WIDTHOF(Bits64) + location.index;
	void *result = (void *)(context->address + index * context->granularity);
	return result;
}

void *PutZeroed(Size size, GranularAllocator *context) {
	void *result = Put(size, context);
	Zero(result, size);
	return result;
}

/* granular allocator / deallocation ******************************************/

void Pop(void *address, Size size, GranularAllocator *context) {
	/* NOTE(Emhyr): #unsafe: we don't check if `address` is valid */
	
	Size count = size / context->granularity;
	Size index = ((Address)address - context->address) / context->granularity;
	
	BitLocation location = {
		.pointer = GetBeginningFlags(context) + index / WIDTHOF(Bits64),
		.index = index % WIDTHOF(Bits64)
	};
	SetBits(count, location, 0, 1);
}


void PopWaned(void *address, Size size, GranularAllocator *context) {
	Assert(!"unimplemented");
}
