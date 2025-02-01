#include "basics.h"

int main(void) {
	GranularAllocator allocator = {0};

	Size n = 1;
	void *p;

	p = Put(n, &allocator);
	Pop(p, n, &allocator);
}
