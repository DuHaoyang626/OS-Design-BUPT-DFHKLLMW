#include "apilib.h"

static void puti(int v)
{
	char s[32];
	int i = 0;
	int sign = 0;
	if (v < 0) { sign = 1; v = -v; }
	if (v == 0) { s[i++] = '0'; }
	while (v > 0 && i < 30) {
		s[i++] = '0' + (v % 10);
		v /= 10;
	}
	if (sign) s[i++] = '-';
	s[i] = 0;
	/* reverse */
	{
		int a = 0, b = i - 1;
		while (a < b) {
			char t = s[a]; s[a] = s[b]; s[b] = t;
			a++; b--;
		}
	}
	api_putstr0(s);
}

static void println(char *s)
{
	api_putstr0(s);
	api_putstr0("\n");
}

void _main()
{
	char buf[128];
	char list[512];
	int r;

	println("[memfstest] format 256KB");
	r = api_memfs_format(256);
	if (r < 0) {
		api_putstr0("format fail: "); puti(r); api_putstr0("\n");
		api_end();
	}

	println("mkdir /dir");
	r = api_memfs_mkdir("/dir");
	api_putstr0("ret="); puti(r); api_putstr0("\n");

	println("create /dir/a.txt");
	r = api_memfs_create("/dir/a.txt");
	api_putstr0("ret="); puti(r); api_putstr0("\n");

	println("write 'hello' to /dir/a.txt");
	{
		char msg[] = "hello";
		r = api_memfs_write("/dir/a.txt", 0, msg, 5);
		api_putstr0("ret="); puti(r); api_putstr0("\n");
	}

	println("read back");
	api_memfs_read("/dir/a.txt", 0, buf, 5);
	buf[5] = 0;
	api_putstr0("buf='"); api_putstr0(buf); api_putstr0("'\n");

	println("copy to /dir/b.txt");
	r = api_memfs_copy("/dir/a.txt", "/dir/b.txt");
	api_putstr0("ret="); puti(r); api_putstr0("\n");

	println("list /dir");
	r = api_memfs_list("/dir", list, 512);
	api_putstr0(list);

	println("delete /dir/a.txt");
	r = api_memfs_delete("/dir/a.txt");
	api_putstr0("ret="); puti(r); api_putstr0("\n");

	println("list /dir");
	api_memfs_list("/dir", list, 512);
	api_putstr0(list);

	api_end();
}
