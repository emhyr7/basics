#if !defined(INCLUDED_BASICS_BITS_H)
#define INCLUDED_BASICS_BITS_H

#include "basics_base.h"

#if defined(COMPILER_IS_CLANG) || defined(COMPILER_IS_GNUC)
#define BitScanForward(x) (__builtin_ffsll(x) - 1)
#elif defined(COMPILER_IS_MSC)
INLINED int BitScanForward(long long int x) {
	int r;
	if (!_BitScanForward64(&r, x)) r = -1;
	return r;
}
#endif

typedef struct {
	Bits64 *pointer;
	Index   index; /* NOTE(Emhyr): index begins at 1 from `*pointer` */
} BitLocation;

PUBLIC BitLocation FindBit (Bits64 *p, Bits64 *q, Boolean clear);
PUBLIC BitLocation FindBits(Size n, Bits64 *p, Bits64 *q, Boolean clear);

PUBLIC void SetBits(Size n, BitLocation location, Boolean clear, Boolean reverse);

#endif
