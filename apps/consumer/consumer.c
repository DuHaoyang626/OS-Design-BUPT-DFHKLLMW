#include "apilib.h"
#include <stdio.h>

void _main()
{
	int i, val;
	char s[32];

	for (i = 0; i < 15; i++) {
		val = api_pc_consume();
		sprintf(s, "Consumed: %d\n", val);
		api_putstr0(s);
	}

	api_end();
}