#include "apilib.h"
#include <stdio.h>

int _main()
{
	int i, j, val;
	char s[40];

	api_putstr0("Starting SAFE test (Semaphore protected)...\n");

	for (i = 0; i < 50000; i++) {
		api_sem_wait();
		val = api_get_shared();
		val = val + 1;

		/* dummy work */
		for (j = 0; j < 500; j++) {}

		api_set_shared(val);
		api_sem_post();

		if (i % 10000 == 0) {
			sprintf(s, "Safe progress: %d\n", val);
			api_putstr0(s);
		}
	}

	val = api_get_shared();
	sprintf(s, "Final Safe Value: %d\n", val);
	api_putstr0(s);

	api_end();
	return 0;
}