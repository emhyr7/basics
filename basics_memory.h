/*
random notes

a collection of memory allocators aware about virtual memory.

ZII-based (Zero Is Initialization).

each allocator can be initialized with given memory or automatically allocate
their own memory.

## glossary

"address"     - a byte index into virtual memory where 0 represents failure.
"extent"      - the amount of addresses allocated.
"commission"  - the amount of addresses committed in virtual memory.
"reservation" - the amount of addresses reserved in virtual memory.
"frame"       - addresses with implicit allocation information.
"block"       - limited allocatable addresses.
"flags"       - flags indicating lock state.

"initialize" - initialize with given arguments.
"create"     - allocate then initialize.
"destroy"    - uninitialize then deallocate.
"make"       - create without expecting deallocation.

"contiguous?" - are the elements stored in consecutive addresses?
"finite?"     - is there a limit to the amount of addresses allocated?
"granular?"   - is each element's size factorable?
"brevity?"    - is the time at which to deallocate known?
"inflative?"  - can the element grow while stationaory?

"push" - increases the extent.
"pull" - decreases the extent.

"put" - locks a block.
"pop" - unlocks a block.

"debug" - do checks and set traps.
"wane"  - after allocating, decommit the next page from the extent's address.
*/

#if !defined(INCLUDED_BASICS_MEMORY_H)
#define INCLUDED_BASICS_MEMORY_H

#include "basics_base.h"

/******************************************************************************/
/* settings */

#if !defined(ENABLE_STENOGRAPHY)
#define ENABLE_STENOGRAPHY 1
#endif

/* when invoking an allocation procedure, the allocator is automatically
initialized by setting zeroed fields to their defaults and preserving
non-zeroed fields */
#if !defined(ENABLE_AUTOMATIC_INITIALIZATION)
#define ENABLE_AUTOMATIC_INITIALIZATION 1
#endif

/* adds `#define`s to alias procedures to their debug variants */
#if !defined(ENABLE_DEBUGGING_ALIASES)
#define ENABLE_DEBUGGING_ALIASES 0
#endif

/* the default factor used for initialization.

a factor is used to multiply the commission upon an allocation. this's useful
to reduce the amount of expensive commits */
#if !defined(DEFAULT_FACTOR)
#define DEFAULT_FACTOR 1
#endif

/* the default commission used for initialization */
#if !defined(DEFAULT_COMMISSION)
#define DEFAULT_COMMISSION 0x1000
#endif

/* the default reservation used for initialization */
#if !defined(DEFAULT_RESERVATION)
#define DEFAULT_RESERVATION 0x40000000
#endif

/* the default granularity used for initialization */
#if !defined(DEFAULT_GRANULARITY)
#define DEFAULT_GRANULARITY 64
#endif

/* the default quantity used for initialization */
#if !defined(DEFAULT_QUANTITY)
#define DEFAULT_QUANTITY 32768
#endif

/******************************************************************************/

PRIVATE INLINED Boolean CheckAlignment(Size alignment) {
	return alignment && !(alignment & (alignment - 1));
}

PRIVATE INLINED Size GaugeBackwardAligner(Size address, Size alignment) {
	Assert(CheckAlignment(alignment), "attempting to use a misalignment is disallowed"); \
	return alignment ? address & (alignment - 1) : 0;
}

PRIVATE INLINED Size GaugeForwardAligner(Size address, Size alignment) {
	Size remainder = GaugeBackwardAligner(address, alignment);
	return remainder ? alignment - remainder : 0;
}

PRIVATE INLINED Size AlignBackwards(Size address, Size alignment) {
	return address - GaugeBackwardAligner(address, alignment);
}

PRIVATE INLINED Size AlignForwards(Size address, Size alignment) {
	return address + GaugeForwardAligner(address, alignment);
}

/******************************************************************************/

Size QueryVirtualMemoryGranularity(void);
Size QueryVirtualMemoryGranularity(void);

Address AllocateVirtualMemory(Size size);

Address ReserveVirtualMemory(Size size);
void    ReleaseVirtualMemory(Address address, Size size);

void CommitVirtualMemory  (Address address, Size size);
void DecommitVirtualMemory(Address address, Size size);

void ValidateVirtualMemory  (Address address, Size size);
void InvalidateVirtualMemory(Address address, Size size);

Boolean CheckCommittedVirtualMemory(Address address, Size size);

void TouchVirtualMemory(Address address, Size size);

/* linear allocator ***********************************************************/

typedef struct {
	Size    reservation;
	Address address;
	Size    factor;
	Size    commission;
	Size    extent;
} LinearAllocator;

typedef LinearAllocator Arena;
typedef LinearAllocator Stack;

#if ENABLE_DEBUGGING_ALIASES
#define InitializeLinearAllocator(...) DebugInitializeLinearAllocator(__VA_ARGS__);
#define MakeLinearAllocator(...)       DebugMakeLinearAllocator(__VA_ARGS__);
#define ClearLinearAllocator(...)      DebugClearLinearAllocator(__VA_ARGS__);
#define ClearLinearAllocatorWaned(...) DebugClearLinearAllocatorWaned(__VA_ARGS__);
#define Push(...)                      DebugPush(__VA_ARGS__);
#define PushZeroed(...)                DebugPushZeroed(__VA_ARGS__);
#define PushFrame(...)                 DebugPushFrame(__VA_ARGS__);
#define PushFrameZeroed(...)           DebugPushFrameZeroed(__VA_ARGS__);
#define Pull(...)                      DebugPull(__VA_ARGS__);
#define PullWaned(...)                 DebugPullWaned(__VA_ARGS__);
#define PullFrame(...)                 DebugPullFrame(__VA_ARGS__);
#define PullFrameWaned(...)            DebugPullFrameWaned(__VA_ARGS__);
#endif

/* linear allocator / creation ************************************************/
PUBLIC void            InitializeLinearAllocator     (LinearAllocator *context);
PUBLIC LinearAllocator MakeLinearAllocator           (Size reservation, Size commission, Size factor);
PUBLIC void            DebugInitializeLinearAllocator(LinearAllocator *context);
PUBLIC LinearAllocator DebugMakeLinearAllocator      (Size reservation, Size commission, Size factor);

/* linear allocator / destruction *********************************************/
PUBLIC void ClearLinearAllocator          (LinearAllocator *context);
PUBLIC void ClearLinearAllocatorWaned     (LinearAllocator *context);
PUBLIC void DebugClearLinearAllocator     (LinearAllocator *context);
PUBLIC void DebugClearLinearAllocatorWaned(LinearAllocator *context);

/* linear allocator / allocation **********************************************/
PUBLIC void *Push                (Size size, Size alignment, LinearAllocator *context);
PUBLIC void *PushZeroed          (Size size, Size alignment, LinearAllocator *context);
PUBLIC void *PushFrame           (Size size, Size alignment, LinearAllocator *context);
PUBLIC void *PushFrameZeroed     (Size size, Size alignment, LinearAllocator *context);
PUBLIC void *DebugPush           (Size size, Size alignment, LinearAllocator *context);
PUBLIC void *DebugPushZeroed     (Size size, Size alignment, LinearAllocator *context);
PUBLIC void *DebugPushFrame      (Size size, Size alignment, LinearAllocator *context);
PUBLIC void *DebugPushFrameZeroed(Size size, Size alignment, LinearAllocator *context);

/* linear allocator / deallocation ********************************************/
PUBLIC void  Pull               (Size size, Size alignment, LinearAllocator *context);
PUBLIC void  PullWaned          (Size size, Size alignment, LinearAllocator *context);
PUBLIC void  PullFrame          (void *address, LinearAllocator *context);
PUBLIC void  PullFrameWaned     (void *address, LinearAllocator *context);
PUBLIC void  DebugPull          (Size size, Size alignment, LinearAllocator *context);
PUBLIC void  DebugPullWaned     (Size size, Size alignment, LinearAllocator *context);
PUBLIC void  DebugPullFrame     (void *address, LinearAllocator *context);
PUBLIC void  DebugPullFrameWaned(void *address, LinearAllocator *context);

/* granular allocator *********************************************************/

typedef struct {
	Size    reservation;
	Address address;
	Size    granularity;
	Size    quantity;
} GranularAllocator;

/* NOTE(Emhyr): etymology: "granular" for consistency with the adjective "linear" in `LinearAllocator` */

typedef GranularAllocator Pool;

/* granular allocator / creation **********************************************/
PUBLIC void              InitializeGranularAllocator(GranularAllocator *context);
PUBLIC GranularAllocator CreateGranularAllocator    (Size reservation, Size granularity, Size quantity);

/* granular allocator / destruction *******************************************/
PUBLIC void ClearGranularAllocator     (GranularAllocator *context);
PUBLIC void ClearGranularAllocatorWaned(GranularAllocator *context);

/* granular allocator / allocation ********************************************/
PUBLIC void *Put      (Size size, GranularAllocator *context);
PUBLIC void *PutZeroed(Size size, GranularAllocator *context);

/* granular allocator / deallocation ******************************************/
PUBLIC void Pop     (void *address, Size size, GranularAllocator *context);
PUBLIC void PopWaned(void *address, Size size, GranularAllocator *context);

#endif
