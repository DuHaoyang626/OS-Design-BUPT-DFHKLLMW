#include "apilib.h"

int _main()
{
	int win, k = 0, wide = 8, high = 24;
	char buf[600 * 440], s[64], i;
	win = api_openwin(buf, 600, 440, 255, "¼ÇÊÂ²¾");
	api_boxfilwin(win, 5, 24, 595, 435, 7);
	
	for (;;) 
	{
		i = api_getkey(1);
		if (wide >= 590)
		{
			wide = 8;
			high += 16;
		}
		if (i == 0x0a)
		{
			high += 16;
			wide = 8;
		}
		else if(i == 0x08)
		{
			if (wide > 8)
			{
				k--;
				wide-=8;
				api_boxfilwin(win, wide, high, wide + 8, high + 16, 7);
			}
			else 
			{
				if (high > 24) 
				{
					high-=16;
					wide = 590;
				}
			}
		}
		else
		{
			s[0] = i;
			k++;
			api_putstrwin(win, wide, high, 0, 1, s);
			api_boxfilwin(win, wide, high, wide + 8, high + 16, 7);
			api_putstrwin(win, wide, high, 0, 1, s);
			wide+=8;
	
		}
	}
	return 0;
}
