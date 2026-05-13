#include <stdio.h>
#include <string.h>
#include "apilib.h"

static void check_ptr(const char *name, void *p, int ds_base)
{
	unsigned int off = (unsigned int) p;
	unsigned int phys = api_virt2phys(p);
	unsigned int expect = (unsigned int) ds_base + off;

	printf("%s: off=0x%08X  virt2phys=0x%08X  expect(base+off)=0x%08X  %s\n",
		name, off, phys, expect, (phys == expect) ? "OK" : "FAIL");
}

int _main(void)
{
	int ds_base;
	char *a;
	char *b;

	api_initmalloc();

	ds_base = api_getdsbase();
	printf("ds_base = 0x%08X\n", (unsigned int) ds_base);

	a = api_malloc(32);
	if (a == 0) {
		api_putstr0("ldtaddr: malloc(a) failed\n");
		api_end();
	}
	strcpy(a, "a=hello_from_a\n");
	check_ptr("a", a, ds_base);
	api_putstr0(a);

	b = api_malloc(32);
	if (b == 0) {
		api_putstr0("ldtaddr: malloc(b) failed\n");
		api_end();
	}
	strcpy(b, "b=hello_from_b\n");
	check_ptr("b", b, ds_base);
	api_putstr0(b);

	printf("b - a (offset diff) = 0x%08X\n", (unsigned int) (b - a));
	printf("note: b offset accounts for a allocation.\n");

	api_end();
	return 0;
}

