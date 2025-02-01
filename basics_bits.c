#include "basics_bits.h"

/* NOTE(Emhyr): "reverse" means to decrement the word pointer. it doesn't scan the bits reversely */

BitLocation FindBit(Bits64 *p, Bits64 *q, Boolean clear) {
	Boolean reverse = q < p;
	Bits64 cmp = clear ? -1 : 0;
	for (;;) {
		if (p == q) goto fail;
		if (*p != cmp) break;
		if (reverse) --p;
		else ++p;
	}
	Bits64 x = *p;
	if (clear) x = ~x;
	return (BitLocation){p, BitScanForward(x)};
fail:
	return (BitLocation){0, 0};
	
#if 0
	/* NOTE(Emhyr): only works with 64 byte strides */
	BitLocation result;
	__mmask8 m;
	__m512i v, cmp;
	unsigned int zcnt;
	
	cmp = _mm512_set1_epi64(clear ? -1 : 0);
	if (reverse) p -= 7;
	for (;;) {
		if (p == q) break;

		v = _mm512_load_epi64(p);
		m = _mm512_cmpneq_epu64_mask(v, cmp);
		if (m) break;

		long long inc = sizeof(v) / sizeof(*p);
		if (reverse) inc = -inc;
		p += inc;
	}

	if (reverse) zcnt = 7 - _lzcnt_u32((unsigned)m << 24);
	else zcnt = _tzcnt_u32(m);
	result.pointer = p + zcnt;
	if (result.pointer != q) {
		Bits64 x = *result.pointer;
		if (clear) x = ~x;
		result.index = BitScanForward(x);
	}
	return result;
#endif
}

/* NOTE(Emhyr): if n <= half a word size, we can buffer-shift bits into a word to
compare with a bitmask. */

BitLocation FindBits(Size n, Bits64 *p, Bits64 *q, Boolean clear) {
	if (n == 1) return FindBit(p, q, clear);
	Boolean reverse = q < p;
	BitLocation result;
	Size c, i, j, z;
	Bits64 w;
	
	result.pointer = p;
	result.index = 0;
	c = 0;
	for (; p != q; reverse ? --p : ++p) {
		w = *p;
		if (clear) w = ~w;
	retry:
		result.pointer = p;
		i = BitScanForward(w);
		if (!i) continue;
		result.index = i - 1;
		w |= ~(-1ll << (i - 1));
		j = BitScanForward(~w);
		if (!j) j = WIDTHOF(*p) + 1;
		c += j - i;
		if (c >= n) break;
		w &= -1ll << (j - 1);
		if (!w) continue;
		if (j == WIDTHOF(*p) + 1) {
			do {
				w = *++p;
				if (!clear) w = ~w;
				if (p == q) goto finished;
				z = _tzcnt_u64(w);
				c += z;
				if (c >= n) goto finished;
			} while (z == WIDTHOF(*p));
			w |= ~(-1ll << (z - 1));
			w = ~w;
		}
		c = 0;
		goto retry;
	}
finished:
	if (p == q) result = (BitLocation){0, 0};
	return result;
}

void SetBits(Size n, BitLocation location, Boolean clear, Boolean reverse) {
	Assert(n);

	/* TODO(Emhyr): discard `n` */
	Bits64 *p, m;
	long long i, k;
	Size c;

	p = location.pointer;
	*p = 0;
	c = WIDTHOF(*location.pointer) - location.index;
	if (n < c) c = n;
	m = ~(c < 64 ? -1ll << c : 0) << location.index;
	if (clear) *p &= ~m;
	else *p |= m;
	if (reverse) --p;
	else ++p;
	n -= c;
	if (!n) return;
	c = WIDTHOF(*location.pointer);
	k = n / c;
	n -= c * k;
	m = clear ? 0 : -1ll;
	for (i = 0; i < k; ++i) {
		*p = m;
		if (reverse) --p;
		else ++p;
	}
	m = -1ll >> (c - n);
	if (clear) *p &= ~m;
	else *p |= m;
}
