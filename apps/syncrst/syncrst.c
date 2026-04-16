#include "apilib.h"

int _main()
{
	api_putstr0("Resetting shared variable & semaphore...\n");
	api_sync_reset();
	api_putstr0("Done.\n");
	api_end();
	return 0;
}