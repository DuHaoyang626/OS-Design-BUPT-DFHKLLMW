#include "apilib.h"

void waittimer(int timer, int time)
{
	int i;
	api_settimer(timer, time);
	for (;;) {
		i = api_getkey(1);
		if (i == 'Q' || i == 'q') {
			api_end();
		}
		if (i == 128) {
			return;
		}
	}
}

void _main()
{
	int i = 0;
	int timer;

	// Automatically open the sync monitor GUI
	api_syncdemo_start();

	// Initialize the producer-consumer state
	api_pc_init();

	timer = api_alloctimer();
	api_inittimer(timer, 128);

	for (;;) {
		api_pc_produce(i);
		i = (i + 1) % 100;
		waittimer(timer, 30);
	}

	api_end();
}