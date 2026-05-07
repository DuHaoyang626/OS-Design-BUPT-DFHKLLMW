#include "apilib.h"
#include <stdio.h>

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
	int val;
	char s[32];
	int timer;

	timer = api_alloctimer();
	api_inittimer(timer, 128);

	for (;;) {
		val = api_pc_consume();
		sprintf(s, "Consumed: %d\n", val);
		api_putstr0(s);
		waittimer(timer, 40); // 消费者稍慢，可观察到写满与等待的情况
	}

	api_end();
}