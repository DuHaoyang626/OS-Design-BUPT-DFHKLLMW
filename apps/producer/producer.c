#include "apilib.h"

void _main()
{
	int i;
	// Initialize the producer-consumer state
	api_pc_init();

	for (i = 0; i < 15; i++) {
		api_pc_produce(i);
	}

	api_end();
}