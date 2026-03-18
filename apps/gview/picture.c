#include <stdio.h>

struct DLL_STRPICENV {
	int work[64 * 1024 / 4];
};

struct RGB {
	unsigned char b, g, r, t;
};

/* bmp.asm */
int info_BMP(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_BMP(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

/* jpeg.c */
int info_JPEG(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_JPEG(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

unsigned char rgb2pal(int r, int g, int b, int x, int y);
void error(char *s);

int main0()
{
	struct DLL_STRPICENV env;
	char filebuf[512 * 1024], winbuf[1040 * 805];
	char s[32], *p;
	int win, i, j, fsize, xsize, info[8];
	struct RGB picbuf[1024 * 768], *q;

	AppCmdline(s, 30);
	for (p = s; *p > ' '; p++) { }
	for (; *p == ' '; p++) { }

	i = AppFopen(p); 
	if (i == 0) 
	{
		error("系统找不到指定的文件。"); 
	}
	
	fsize = AppFsize(i, 0);
	if (fsize > 512 * 1024) 
	{
		error("file too large.\n");
	}
	AppFread(filebuf, fsize, i);
	AppFclose(i);

	if (info_BMP(&env, info, fsize, filebuf) == 0) 
	{
		if (info_JPEG(&env, info, fsize, filebuf) == 0) 
		{
			error("图片格式未知或图片已损坏。");
		}
	}

	if (info[2] > 1024 || info[3] > 768) 
	{
		error("picture too large.");
	}

	xsize = info[2] + 16;
	if (xsize < 224) 
	{
		xsize = 224;
	}
	win = WinMake(winbuf, xsize, info[3] + 53, 255, "图片");
	
	sprintf(s, "%s      %4dx%d", p, info[2], info[3]);
	WinPutstr(win, 8, info[3] + 31, 0, 28, s);

	if (info[0] == 1) 
	{
		i = decode0_BMP (&env, fsize, filebuf, 4, (char *) picbuf, 0);
	}
	else 
	{
		i = decode0_JPEG(&env, fsize, filebuf, 4, (char *) picbuf, 0);
	}
	if (i != 0) 
	{
		error("decode error.\n");
	}

	for (i = 0; i < info[3]; i++) 
	{
		p = winbuf + (i + 29) * xsize + (xsize - info[2]) / 2;
		q = picbuf + i * info[2];
		for (j = 0; j < info[2]; j++)
		{
			p[j] = rgb2pal(q[j].r, q[j].g, q[j].b, j, i);
		}
	}
	WinRefresh(win, (xsize - info[2]) / 2, 29, (xsize - info[2]) / 2 + info[2], 29 + info[3]);

	for (;;) {}
	
	return 0;
}

unsigned char rgb2pal(int r, int g, int b, int x, int y)
{
	static int table[4] = { 3, 1, 0, 2 };
	int i;
	
	x &= 1;
	y &= 1;
	i = table[x + y * 2];
	r = (r * 21) / 256;
	g = (g * 21) / 256;
	b = (b * 21) / 256;
	r = (r + i) / 4;
	g = (g + i) / 4;
	b = (b + i) / 4;
	
	return 16 + r + g * 6 + b * 36;
}

void error(char *s)
{
	int win, win2;
	char winbuf[460 * 300], winbuf2[256 * 48];
	win = WinMake(winbuf, 460, 300, 255, "图片");
	win2 = WinMake(winbuf2, 256, 48, 255, "错误");
	WinPutstr(win2, 24, 24, 0, 32, s);
	for (;;) {}
}
