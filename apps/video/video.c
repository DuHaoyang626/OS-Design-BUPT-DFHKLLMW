#include "apilib.h"

#include <string.h>	/* strlen */
#include <stdio.h>

int strtol(char *s, char **endp, int base);	 /*标准函数<stdlib.h> */

void waittimer(int timer, int time);

int _main()
{
	char winbuf[400 * 300], txtbuf[100 * 1024];
	char s[32], *p, *r;
	int win, x = 64, y = 64, timer, i, j;
	
	/*准备窗口*/
	win = api_openwin(winbuf, 400, 300, 255, "视频");

	/*命令行解析*/
	api_cmdline(s, 30);

	for (p = s; *p > ' '; p++) { }	//一直读到空格为止
	for (; *p == ' '; p++) { }	//跳过空格
	i = strlen(p);
	if (i > 12 || i == 0) 
	{
file_error:
	api_putstrwin(win, 5/*左右位置*/, 26/*上下位置*/, 0/*颜色:黑*/, 50/*文字宽度*/, "系统找不到指定文件或文件错误！");/*文字内容*/
		for (;;) {}
	}
	
	/*载入文件*/
	i = api_fopen(p);
	if (i == 0) 
	{
		goto file_error;
	}
	j = api_fsize(i, 0);
	if (j >= 100 * 1024)
	{
		j = 100 * 1024 - 1;
	}
	api_fread(txtbuf, j, i);
	api_fclose(i);
	txtbuf[j] = 0;
	r = txtbuf;
	i = 0; /*通常模式*/
	
	api_putstrwin(win, 16, 24, 3, 12, p);
	
	for (p = txtbuf; *p != 0; p++) {	//为了方便处理，将注释和空白删去
		if (i == 0 && *p > ' ') {		//不是空格或换行符
			if (*p == '/') {
				if (p[1] == '*') {
					i = 1;
				} else if (p[1] == '/') {
					i = 2;
				} else {
					*r = *p;
					if ('a' <= *p && *p <= 'z') {
						*r += 'A' - 'a';	//将小写字母转换为大写字母
					}
					r++;
				}
			}
			else if (*p == 0x22) 
			{
				*r = *p;
				r++;
				i = 3;
			}
			else 
			{
				*r = *p;
				r++;
			}
		}
		else if (i == 1 && *p == '*' && p[1] == '/') 
		{	//段注释
			p++;
			i = 0;
		} 
		else if (i == 2 && *p == 0x0a) 
		{	//行注释
			i = 0;
		}
		else if (i == 3) 
		{	//字符串
			*r = *p;
			r++;
			if (*p == 0x22) {
				i = 0;
			}
			else if (*p == '%')
			{
				p++;
				*r = *p;
				r++;
			}
		}
	}
	*r = 0;

	/*定时器准备*/
	timer = api_alloctimer();
	api_inittimer(timer, 128);

	/*主体*/
	p = txtbuf;
	for (;;) {
		if (*p == '&') 
		{
			for (;;) {}
		}
		else if (*p == '.')
		{
			if (y > 128) {
				y = 64;
			}
			if (x < 127) {
				x++;
			} else {
				y++;
				x = 64;
			}
			api_point(win, x, y, 7);
		}
		else if (*p == '0')
		{
			if (y > 128) {
				y = 64;
			}
			if (x < 127) {
				x++;
			} else {
				y++;
				x = 64;
			}
			api_point(win, x, y, 0);
		}
		else if (*p == 0) 
		{
			x = 64;
			p = txtbuf;
		} 
		p++;
	}
	return 0;
}

void waittimer(int timer, int time)
{
	int i;
	api_settimer(timer, time);
	for (;;) 
	{
		i = api_getkey(1);
		if (i == 128) 
		{
			return;
		}
	}
}

