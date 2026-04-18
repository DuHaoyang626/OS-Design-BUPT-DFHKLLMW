/* --------------------------------
	B Y : S T O N
	HELO OS 核心文件
	    ver. 1.0
         DATE : 2019-1-19  
----------------------------------- */

#include "bootpack.h"
#include <stdio.h>
#include <string.h>
#include "../apps/stdlib.h"

/* memfs console commands */
static void cmd_mformat(struct CONSOLE *cons, char *cmdline);
static void cmd_mmkdir(struct CONSOLE *cons, char *cmdline);
static void cmd_mcreate(struct CONSOLE *cons, char *cmdline);
static void cmd_mwrite(struct CONSOLE *cons, char *cmdline);
static void cmd_mread(struct CONSOLE *cons, char *cmdline);
static void cmd_mcopy(struct CONSOLE *cons, char *cmdline);
static void cmd_mls(struct CONSOLE *cons, char *cmdline);
static void cmd_mrm(struct CONSOLE *cons, char *cmdline);
static void cmd_memfs_help(struct CONSOLE *cons);

void console_task(struct SHEET *sheet, int memtotal)
{
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct CONSOLE cons;
	struct FILEHANDLE fhandle[8];
	char cmdline[30];
	unsigned char *nihongo = (char *) *((int *) 0x0fe8);

	cons.sht = sheet;
	cons.cur_x =  8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;
	task->cmdline = cmdline;

	if (cons.sht != 0) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	for (i = 0; i < 8; i++) {
		fhandle[i].buf = 0;
	}
	task->fhandle = fhandle;
	task->fat = fat;
	if (nihongo[4096] != 0xff) {
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;
	task->langmode = 3;//表示每次都选择汉字
	cons_putchar(&cons, '#', 1);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1 && cons.sht != 0) {
				if (i != 0) {
					timer_init(cons.timer, &task->fifo, 0);
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(cons.timer, &task->fifo, 1);
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(cons.timer, 50);
			}
			if (i == 2) {
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) {
				if (cons.sht != 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				cons.cur_c = -1;
			}
			if (i == 4) {
				cmd_exit(&cons, fat);
			}
			if (256 <= i && i <= 511) {
				if (i == 8 + 256) {
					if (cons.cur_x > 16) {
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (i == 10 + 256) {
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);
					if (cons.sht == 0) {
						cmd_exit(&cons, fat);
					}
					cons_putchar(&cons, '#', 1);
				} else {
					if (cons.cur_x < 512) //主窗口x轴大小
					{
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						cons_putchar(&cons, i - 256, 1);
					}
				}
			}
			if (cons.sht != 0) {
				if (cons.cur_c >= 0) {
					boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
			}
		}
	}
}

/* =======================================
窗口大小调整，主要是x轴
========================================== */
void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if (s[0] == 0x09) {
		for (;;) {
			if (cons->sht != 0) {
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 512) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;
			}
		}
	} else if (s[0] == 0x0a) {
		cons_newline(cons);
	} else if (s[0] == 0x0d) {
	} else {
		if (cons->sht != 0) {
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move != 0) {
			cons->cur_x += 8;
			if (cons->cur_x == 8 + 512) {
				cons_newline(cons);
			}
		}
	}
	return;
}
/* =======================================
窗口大小调整，x轴和y轴大小
========================================== */
void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	struct TASK *task = task_now();
	//if (cons->cur_y < 28 + 112) {
	if (cons->cur_y < 28 + 432) {
		cons->cur_y += 16;
	} else {
		if (sheet != 0) {
			//for (y = 28; y < 28 + 112; y++) {
			for (y = 28; y < 28 + 432; y++) {
				for (x = 8; x < 8 + 512; x++) {
					sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
				}
			}
			//for (y = 28 + 112; y < 28 + 128; y++) {
			for (y = 28 + 432; y < 28 + 448; y++) {
				for (x = 8; x < 8 + 512; x++) {
					sheet->buf[x + y * sheet->bxsize] = COL8_000000;
				}
			}
			//sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
			sheet_refresh(sheet, 8, 28, 8 + 512, 28 + 448);
		}
	}
	cons->cur_x = 8;
	if (task->langmode == 1 && task->langbyte1 != 0) {
		cons->cur_x = 16;
	}
	return;
}

void cons_putstr0(struct CONSOLE *cons, char *s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
	int i;
	for (i = 0; i < l; i++) {
		cons_putchar(cons, s[i], 1);
	}
	return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
	if (strcmp(cmdline, "memfs") == 0 && cons->sht != 0) {
		cmd_memfs_help(cons);
	} else if (strncmp(cmdline, "mformat ", 8) == 0 && cons->sht != 0) {
		cmd_mformat(cons, cmdline);
	} else if (strncmp(cmdline, "mmkdir ", 7) == 0 && cons->sht != 0) {
		cmd_mmkdir(cons, cmdline);
	} else if (strncmp(cmdline, "mcreate ", 8) == 0 && cons->sht != 0) {
		cmd_mcreate(cons, cmdline);
	} else if (strncmp(cmdline, "mwrite ", 7) == 0 && cons->sht != 0) {
		cmd_mwrite(cons, cmdline);
	} else if (strncmp(cmdline, "mread ", 6) == 0 && cons->sht != 0) {
		cmd_mread(cons, cmdline);
	} else if (strncmp(cmdline, "mcopy ", 6) == 0 && cons->sht != 0) {
		cmd_mcopy(cons, cmdline);
	} else if (strncmp(cmdline, "mls ", 4) == 0 && cons->sht != 0) {
		cmd_mls(cons, cmdline);
	} else if (strncmp(cmdline, "mrm ", 4) == 0 && cons->sht != 0) {
		cmd_mrm(cons, cmdline);
	} else if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "cls") == 0 && cons->sht != 0) {
		cmd_cls(cons);
	} else if (strcmp(cmdline, "help") == 0 && cons->sht != 0) {
		cmd_help(cons);
	} else if (strcmp(cmdline, "ver") == 0 && cons->sht != 0) {
		cmd_ver(cons);
	} else if (strcmp(cmdline, "dir") == 0 && cons->sht != 0) {
		cmd_dir(cons);
	} else if (strcmp(cmdline, "ls") == 0 && cons->sht != 0) {
		cmd_dir(cons);
	}else if (strcmp(cmdline, "shutdown")== 0) {
		shutdown();
	} else if (strcmp(cmdline, "clear") == 0 && cons->sht != 0) {
		cmd_cls(cons);
	} else if (strcmp(cmdline, "exit") == 0) {
		cmd_exit(cons, fat);
	} else if (strncmp(cmdline, "start", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	} else if (strncmp(cmdline, "ncst", 5) == 0) {
		cmd_ncst(cons, cmdline, memtotal);
	} else if (cmdline[0] != 0) {
		if (cmd_app(cons, fat, cmdline) == 0) {
			cons_putstr0(cons, "\n您输入命令的既不是何乐操作系统内部指令，也不是外部程序。\n\n");
		}
	}
	return;
}

void cmd_ver(struct CONSOLE *cons)
{
	cons_putstr0(cons, "\n");
	cons_putstr0(cons, "Helo_OS v4.1   <shell 5.2>  GUI 2.2\n");
	cons_putstr0(cons, "Copyright (C) 2019 PengZekai\n");
	cons_putstr0(cons, "[issue]     发布版\n\n");
	return;
}

void cmd_help(struct CONSOLE *cons)
{
	cons_putstr0(cons, "\n\n");
	cons_putstr0(cons, "命令            功能\n");
	cons_putstr0(cons, "mem             查看内存\n");
	cons_putstr0(cons, "mfstest         memfs\xB2\xE2\xCA\xD4\xB3\xCC\xD0\xF2\n");
	cons_putstr0(cons, "memfs           \xCF\xD4\xCA\xBEmemfs\xC3\xFC\xC1\xEE\xB0\xEF\xD6\xFA\n");
	cons_putstr0(cons, "tview           文件阅读器\n");
	cons_putstr0(cons, "gview           图片查看器\n");
	cons_putstr0(cons, "cls             清屏\n");
	cons_putstr0(cons, "dir             文件目录\n");
	cons_putstr0(cons, "couture         秒表\n");
	cons_putstr0(cons, "ls              文件目录\n");
	cons_putstr0(cons, "music           音乐播放器\n");
	cons_putstr0(cons, "type            命令行查看\n");
	cons_putstr0(cons, "calc            命令行计算器\n");
	cons_putstr0(cons, "请键入Tview help.txt -w70 -h30\n获取更多的帮助\n\n");
	return;
}

static void cmd_memfs_help(struct CONSOLE *cons)
{
	cons_putstr0(cons, "\nmemfs \xC3\xFC\xC1\xEE\xB0\xEF\xD6\xFA\n");
	cons_putstr0(cons, "mformat <kb>     memfs\xB8\xF1\xCA\xBD\xBB\xAF\x28\xB5\xA5\xCE\xBB\x4B\x42\x29\n");
	cons_putstr0(cons, "mmkdir <path>    memfs\xB4\xB4\xBD\xA8\xC4\xBF\xC2\xBC\n");
	cons_putstr0(cons, "mcreate <path>   memfs\xB4\xB4\xBD\xA8\xBF\xD5\xCE\xC4\xBC\xFE\n");
	cons_putstr0(cons, "mwrite <path> <off> <text>  memfs\xD0\xB4\xC8\xEB\xCE\xC4\xB1\xBE\n");
	cons_putstr0(cons, "mread <path> <off> <len>    memfs\xB6\xC1\xC8\xA1\xB2\xA2\xCF\xD4\xCA\xBE\n");
	cons_putstr0(cons, "mcopy <src> <dst> memfs\xB8\xB4\xD6\xC6\xCE\xC4\xBC\xFE\n");
	cons_putstr0(cons, "mls <path>       memfs\xC1\xD0\xC4\xBF\xC2\xBC\n");
	cons_putstr0(cons, "mrm <path>       memfs\xC9\xBE\xB3\xFD\xCE\xC4\xBC\xFE\x2F\xC4\xBF\xC2\xBC\n\n");
}

static void cmd_memfs_putret(struct CONSOLE *cons, int ret)
{
	char s[64];
	if (ret >= 0) {
		sprintf(s, "[OK] ret=%d\n", ret);
		cons_putstr0(cons, s);
		return;
	}
	cons_putstr0(cons, "[ERR] ");
	sprintf(s, "ret=%d ", ret);
	cons_putstr0(cons, s);
	switch (ret) {
		case -1:
			cons_putstr0(cons, "EINVAL\n");
			break;
		case -2:
			cons_putstr0(cons, "ENOSPACE\n");
			break;
		case -3:
			cons_putstr0(cons, "NOTFOUND\n");
			break;
		case -4:
			cons_putstr0(cons, "EXISTS\n");
			break;
		case -5:
			cons_putstr0(cons, "NOTDIR\n");
			break;
		case -6:
			cons_putstr0(cons, "ISDIR\n");
			break;
		default:
			cons_putstr0(cons, "ERROR\n");
			break;
	}
}

static int cmd_memfs_skipsp(const char *s)
{
	int i = 0;
	while (s[i] == ' ' || s[i] == '\t') {
		i++;
	}
	return i;
}

static int cmd_memfs_token(const char *s, int i, char *out, int outsz)
{
	int j = 0;
	i += cmd_memfs_skipsp(s + i);
	while (s[i] != 0 && s[i] != ' ' && s[i] != '\t') {
		if (j + 1 < outsz) {
			out[j++] = s[i];
		}
		i++;
	}
	out[j] = 0;
	return i;
}

static int cmd_memfs_atoi(const char *s)
{
	int i = 0;
	int sign = 1;
	int val = 0;
	if (s == 0) {
		return 0;
	}
	while (s[i] == ' ' || s[i] == '\t') {
		i++;
	}
	if (s[i] == '-') {
		sign = -1;
		i++;
	} else if (s[i] == '+') {
		i++;
	}
	while (s[i] >= '0' && s[i] <= '9') {
		val = val * 10 + (s[i] - '0');
		i++;
	}
	return sign * val;
}

static void cmd_mformat(struct CONSOLE *cons, char *cmdline)
{
	char a[32];
	int kb, ret;
	cmd_memfs_token(cmdline, 7, a, sizeof(a));
	if (a[0] == 0) {
		cons_putstr0(cons, "\xD3\xC3\xB7\xA8\x3A\x20mformat <kb>\n");
		return;
	}
	kb = cmd_memfs_atoi(a);
	cons_putstr0(cons, "mformat ");
	cons_putstr0(cons, a);
	cons_putstr0(cons, " KB\n");
	ret = memfs_format(kb);
	cmd_memfs_putret(cons, ret);
}

static void cmd_mmkdir(struct CONSOLE *cons, char *cmdline)
{
	char path[128];
	int ret;
	cmd_memfs_token(cmdline, 6, path, sizeof(path));
	if (path[0] == 0) {
		cons_putstr0(cons, "\xD3\xC3\xB7\xA8\x3A\x20mmkdir <path>\n");
		return;
	}
	cons_putstr0(cons, "mmkdir ");
	cons_putstr0(cons, path);
	cons_putstr0(cons, "\n");
	ret = memfs_mkdir(path);
	cmd_memfs_putret(cons, ret);
}

static void cmd_mcreate(struct CONSOLE *cons, char *cmdline)
{
	char path[128];
	int ret;
	cmd_memfs_token(cmdline, 7, path, sizeof(path));
	if (path[0] == 0) {
		cons_putstr0(cons, "\xD3\xC3\xB7\xA8\x3A\x20mcreate <path>\n");
		return;
	}
	cons_putstr0(cons, "mcreate ");
	cons_putstr0(cons, path);
	cons_putstr0(cons, "\n");
	ret = memfs_create(path);
	cmd_memfs_putret(cons, ret);
}

static void cmd_mls(struct CONSOLE *cons, char *cmdline)
{
	char path[128];
	char out[512];
	int ret;
	cmd_memfs_token(cmdline, 3, path, sizeof(path));
	if (path[0] == 0) {
		cons_putstr0(cons, "\xD3\xC3\xB7\xA8\x3A\x20mls <path>\n");
		return;
	}
	cons_putstr0(cons, "mls ");
	cons_putstr0(cons, path);
	cons_putstr0(cons, "\n");
	ret = memfs_list(path, out, sizeof(out));
	cmd_memfs_putret(cons, ret);
	if (ret >= 0) {
		if (ret >= (int) sizeof(out)) {
			ret = sizeof(out) - 1;
		}
		out[ret] = 0;
		if (ret == 0 || out[0] == 0) {
			cons_putstr0(cons, "(empty)\n");
		} else {
			cons_putstr0(cons, out);
			if (out[ret - 1] != '\n') {
				cons_putstr0(cons, "\n");
			}
		}
	}
}

static void cmd_mrm(struct CONSOLE *cons, char *cmdline)
{
	char path[128];
	int ret;
	cmd_memfs_token(cmdline, 3, path, sizeof(path));
	if (path[0] == 0) {
		cons_putstr0(cons, "\xD3\xC3\xB7\xA8\x3A\x20mrm <path>\n");
		return;
	}
	cons_putstr0(cons, "mrm ");
	cons_putstr0(cons, path);
	cons_putstr0(cons, "\n");
	ret = memfs_delete(path);
	cmd_memfs_putret(cons, ret);
}

static void cmd_mcopy(struct CONSOLE *cons, char *cmdline)
{
	char src[128];
	char dst[128];
	int i;
	int ret;
	i = cmd_memfs_token(cmdline, 5, src, sizeof(src));
	cmd_memfs_token(cmdline, i, dst, sizeof(dst));
	if (src[0] == 0 || dst[0] == 0) {
		cons_putstr0(cons, "\xD3\xC3\xB7\xA8\x3A\x20mcopy <src> <dst>\n");
		return;
	}
	cons_putstr0(cons, "mcopy ");
	cons_putstr0(cons, src);
	cons_putstr0(cons, " -> ");
	cons_putstr0(cons, dst);
	cons_putstr0(cons, "\n");
	ret = memfs_copy(src, dst);
	cmd_memfs_putret(cons, ret);
}

static void cmd_mwrite(struct CONSOLE *cons, char *cmdline)
{
	char path[128];
	char off_s[32];
	int i;
	int off;
	const char *text;
	int ret;
	i = cmd_memfs_token(cmdline, 6, path, sizeof(path));
	i = cmd_memfs_token(cmdline, i, off_s, sizeof(off_s));
	if (path[0] == 0 || off_s[0] == 0) {
		cons_putstr0(cons, "\xD3\xC3\xB7\xA8\x3A\x20mwrite <path> <off> <text>\n");
		return;
	}
	off = cmd_memfs_atoi(off_s);
	i += cmd_memfs_skipsp(cmdline + i);
	text = cmdline + i;
	if (*text == 0) {
		cons_putstr0(cons, "\xD3\xC3\xB7\xA8\x3A\x20mwrite <path> <off> <text>\n");
		return;
	}
	cons_putstr0(cons, "mwrite ");
	cons_putstr0(cons, path);
	cons_putstr0(cons, " off=");
	cons_putstr0(cons, off_s);
	cons_putstr0(cons, " len=");
	{
		char s[32];
		sprintf(s, "%d", (int) strlen(text));
		cons_putstr0(cons, s);
	}
	cons_putstr0(cons, "\n");
	ret = memfs_write(path, off, (char *) text, strlen(text));
	cmd_memfs_putret(cons, ret);
}

static void cmd_mread(struct CONSOLE *cons, char *cmdline)
{
	char path[128];
	char off_s[32];
	char len_s[32];
	char buf[256];
	int i;
	int off, len, ret;
	i = cmd_memfs_token(cmdline, 5, path, sizeof(path));
	i = cmd_memfs_token(cmdline, i, off_s, sizeof(off_s));
	cmd_memfs_token(cmdline, i, len_s, sizeof(len_s));
	if (path[0] == 0 || off_s[0] == 0 || len_s[0] == 0) {
		cons_putstr0(cons, "\xD3\xC3\xB7\xA8\x3A\x20mread <path> <off> <len>\n");
		return;
	}
	off = cmd_memfs_atoi(off_s);
	len = cmd_memfs_atoi(len_s);
	if (len < 0) {
		len = 0;
	}
	if (len > (int) (sizeof(buf) - 1)) {
		len = sizeof(buf) - 1;
	}
	cons_putstr0(cons, "mread ");
	cons_putstr0(cons, path);
	cons_putstr0(cons, " off=");
	cons_putstr0(cons, off_s);
	cons_putstr0(cons, " len=");
	cons_putstr0(cons, len_s);
	cons_putstr0(cons, "\n");
	ret = memfs_read(path, off, buf, len);
	cmd_memfs_putret(cons, ret);
	if (ret > 0) {
		buf[ret] = 0;
		cons_putstr0(cons, "data: ");
		cons_putstr0(cons, buf);
		cons_putstr0(cons, "\n");
	} else if (ret == 0) {
		cons_putstr0(cons, "(empty)\n");
	}
}

void cmd_mem(struct CONSOLE *cons, int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[96];
	long int notfree = memtotal / 1048576 - memman_total(memman) / 1048576;
	sprintf(s, "\n内存总量：  %dMB\n可用内存：  %dMB\n已用内存：  %dMB\nALGO: %s\n\n", memtotal / 1048576, memman_total(memman) / 1048576, notfree, memman_get_algo_name());
	cons_putstr0(cons, s);
	return;
}

//改窗口大小后cls命令也要调整参数
void cmd_cls(struct CONSOLE *cons)
{
	int x, y;
	struct SHEET *sheet = cons->sht;
	for (y = 28; y < 28 + 448; y++) {
		for (x = 8; x < 8 + 512; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 512, 28 + 448);
	cons->cur_y = 28;
	return;
}
//dir命令
void cmd_dir(struct CONSOLE *cons)
{
	struct TASK *task = task_now();
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j, k = 0, l;
	char s[60];
	for (i = 0; i < 224; i++) {
		if (k > 400) {
			cons_putstr0(cons, "\n文件过多，请按任意键继续。。。");
			do 
			{
				l = fifo32_get(&task->fifo);
			} while (256 > l || l > 511);
			cons_putstr0(cons, "\n\n");
			k = 0;
		}
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				k += 16;
				sprintf(s, "                ext文件       %7d字节\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
					if (s[j] == 0)
					{
						break;
					}
				}
				s[j] = '.';
				s[j+1] = finfo[i].ext[0];
				s[j+2] = finfo[i].ext[1];
				s[j+3] = finfo[i].ext[2];
				
				s[16] = finfo[i].ext[0];
				s[17] = finfo[i].ext[1];
				s[18] = finfo[i].ext[2];
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_exit(struct CONSOLE *cons, int *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	if (cons->sht != 0) {
		timer_cancel(cons->timer);
	}
	memman_free_4k(memman, (int) fat, 4 * 2880);
	io_cli();
	if (cons->sht != 0) {
		fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768);
	} else {
		fifo32_put(fifo, task - taskctl->tasks0 + 1024);
	}
	io_sti();
	for (;;) {
		task_sleep(task);
	}
}

void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_console(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct TASK *task = open_constask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	/* Enter */
	cons_newline(cons);
	return;
}

//设置应用程序
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char name[18], *p, *q;
	struct TASK *task = task_now();
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	struct SHTCTL *shtctl;
	struct SHEET *sht;
	for (i = 0; i < 13; i++) {
		if (cmdline[i] <= ' ') {
			break;
		}
		name[i] = cmdline[i];
	}
	name[i] = 0;
	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {
		// ------------------------------------------------
		name[i    ] = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'E';
		name[i + 3] = 'L';
		name[i + 4] = 0;
		// ------------------------------------------------
		finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	}

	if (finfo != 0) {
		appsiz = finfo->size;
		p = file_loadfile2(finfo->clustno, &appsiz, fat);
		// -----------------------------------------------
		if (appsiz >= 36 && strncmp(p + 4, "Helo！", 4) == 0 && *p == 0x00) 
		// -----------------------------------------------
		{
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(memman, segsiz);
			task->ds_base = (int) q;
			set_segmdesc(task->ldt + 0, appsiz - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1, (int) q, AR_DATA32_RW + 0x60);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
			shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
			for (i = 0; i < MAX_SHEETS; i++) {
				sht = &(shtctl->sheets0[i]);
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
					sheet_free(sht);
				}
			}
			for (i = 0; i < 8; i++) {
				if (task->fhandle[i].buf != 0) {
					memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int) q, segsiz);
			task->langbyte1 = 0;
		} else {
			cons_putstr0(cons, "Helo OS 应用程序文件打开错误，或者不是标准的Helo os可执行文件！\n所以无法在本计算机上运行 !\n.HEL application program Opening Error.\n");
		}
		memman_free_4k(memman, (int) p, appsiz);
		cons_newline(cons);
		return 1;
	}
	return 0;
}

#define RTC_SECOND			0x00
#define RTC_MINUTE			0x02
#define RTC_HOURS			0x04
#define RTC_WEEKDAY			0x06
#define RTC_DAY_OF_MONTH	0x07
#define RTC_MONTH 			0x08
#define RTC_YEAR			0x09
#define RTC_CENTURY			0x32
#define RTC_REG_A			0x0A
#define RTC_REG_B			0x0b

int get_rtc_register(char address)
{
	int value = 0;
	io_cli();
	io_out8(0x70, address);
	value = io_in8(0x71);
	io_sti();
	return value;
}

int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct TASK *task = task_now();
	int ds_base = task->ds_base;
	struct CONSOLE *cons = task->cons;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht;
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	int *reg = &eax + 1;
	int i, j;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	switch (edx) {
		case 1:
			cons_putchar(cons, eax & 0xff, 1);
			break;
		case 2:
			cons_putstr0(cons, (char *) ebx + ds_base);
			break;
		case 3:
			cons_putstr1(cons, (char *) ebx + ds_base, ecx);
			break;
		case 4:
			return &(task->tss.esp0);
		case 5:
			sht = sheet_alloc(shtctl);
			sht->task = task;
			sht->flags |= 0x10;
			sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
			make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
			sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
			sheet_updown(sht, shtctl->top); /*将窗口图层高度指定为当前鼠标所在图层的高度，鼠标移到上层*/
			reg[7] = (int) sht;
			break;
		case 6:
			sht = (struct SHEET *) (ebx & 0xfffffffe);
			putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
			if ((ebx & 1) == 0) {
				sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
			}
			break;
		case 7:
			sht = (struct SHEET *) (ebx & 0xfffffffe);
			boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
			if ((ebx & 1) == 0) {
				sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
			}
			break;
		case 8:
			memman_init((struct MEMMAN *) (ebx + ds_base));
			ecx &= 0xfffffff0; /*以16字节为单位*/
			memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
			break;
		case 9:
			ecx = (ecx + 0x0f) & 0xfffffff0; /*以16字节为单位进位取整*/
			reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
			break;
		case 10:
			ecx = (ecx + 0x0f) & 0xfffffff0; /*以16字节为单位进位取整*/
			memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
			break;
		case 11:
			sht = (struct SHEET *) (ebx & 0xfffffffe);
			sht->buf[sht->bxsize * edi + esi] = eax;
			if ((ebx & 1) == 0) {
				sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
			}
			break;
		case 12:
			sht = (struct SHEET *) ebx;
			sheet_refresh(sht, eax, ecx, esi, edi);
			break;
		case 13:
			sht = (struct SHEET *) (ebx & 0xfffffffe);
			hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
			if ((ebx & 1) == 0) {
				if (eax > esi) {
					i = eax;
					eax = esi;
					esi = i;
				}
				if (ecx > edi) {
					i = ecx;
					ecx = edi;
					edi = i;
				}
				sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
			}
			break;
		case 14:
			sheet_free((struct SHEET *) ebx);
			break;
		case 15:
			for (;;) {
				io_cli();
				if (fifo32_status(&task->fifo) == 0) {
					if (eax != 0) {
						task_sleep(task); /* FIFO为空，休眠并等待*/
					} else {
						io_sti();
						reg[7] = -1;
						return 0;
					}
				}
				i = fifo32_get(&task->fifo);
				io_sti();
				if (i <= 1) { /*光标用定时器*/
					/*应用程序运行时不需要显示光标，因此总是将下次显示用的值置为1*/
					timer_init(cons->timer, &task->fifo, 1); /*下次置为1*/
					timer_settime(cons->timer, 50);
				}
				if (i == 2) { /*光标ON */
					cons->cur_c = COL8_FFFFFF;
				}
				if (i == 3) { /*光标OFF */
					cons->cur_c = -1;
				}
				if (i == 4) { /*只关闭命令行窗口*/
					timer_cancel(cons->timer);
					io_cli();
					fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024); /*2024～2279*/
					cons->sht = 0;
					io_sti();
				}
				if (i >= 256) { /*键盘数据（通过任务A）等*/
					reg[7] = i - 256;
					return 0;
				}
			}
			break;
		case 16:
			reg[7] = (int) timer_alloc();
			((struct TIMER *) reg[7])->flags2 = 1; /*允许自动取消*/
			break;
		case 17:
			timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
			break;
		case 18:
			timer_settime((struct TIMER *) ebx, eax);
			break;
		case 19:
			timer_free((struct TIMER *) ebx);
			break;
		case 20:
			if (eax == 0) {
				i = io_in8(0x61);
				io_out8(0x61, i & 0x0d);
			} else {
				i = 1193180000 / eax;
				io_out8(0x43, 0xb6);
				io_out8(0x42, i & 0xff);
				io_out8(0x42, i >> 8);
				i = io_in8(0x61);
				io_out8(0x61, (i | 0x03) & 0x0f);
			}
			break;
		case 21:
			for (i = 0; i < 8; i++) {
				if (task->fhandle[i].buf == 0) {
					break;
				}
			}
			fh = &task->fhandle[i];
			reg[7] = 0;
			if (i < 8) {
				finfo = file_search((char *) ebx + ds_base,
						(struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
				if (finfo != 0) {
					reg[7] = (int) fh;
					fh->size = finfo->size;
					fh->pos = 0;
					fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
				}
			}
			break;
		case 22:
			fh = (struct FILEHANDLE *) eax;
			memman_free_4k(memman, (int) fh->buf, fh->size);
			fh->buf = 0;
			break;
		case 23:
			fh = (struct FILEHANDLE *) eax;
			if (ecx == 0) {
				fh->pos = ebx;
			} else if (ecx == 1) {
				fh->pos += ebx;
			} else if (ecx == 2) {
				fh->pos = fh->size + ebx;
			}
			if (fh->pos < 0) {
				fh->pos = 0;
			}
			if (fh->pos > fh->size) {
				fh->pos = fh->size;
			}
			break;
		case 24:
			fh = (struct FILEHANDLE *) eax;
			if (ecx == 0) {
				reg[7] = fh->size;
			} else if (ecx == 1) {
				reg[7] = fh->pos;
			} else if (ecx == 2) {
				reg[7] = fh->pos - fh->size;
			}
			break;
		case 25:
			fh = (struct FILEHANDLE *) eax;
			for (i = 0; i < ecx; i++) {
				if (fh->pos == fh->size) {
					break;
				}
				*((char *) ebx + ds_base + i) = fh->buf[fh->pos];
				fh->pos++;
			}
			reg[7] = i;
			break;
		case 26:
			i = 0;
			for (;;) {
				*((char *) ebx + ds_base + i) =  task->cmdline[i];
				if (task->cmdline[i] == 0) {
					break;
				}
				if (i >= ecx) {
					break;
				}
				i++;
			}
			reg[7] = i;
			break;
		case 27:
			reg[7] = task->langmode;
			break;
		case 28:
			// Make sure an update isn't in progress
			while (get_rtc_register(RTC_REG_A) & 0x80);
			i = get_rtc_register(eax);
			// Convert BCD to binary values if necessary
			j = get_rtc_register(RTC_REG_B);
			if (!(j & 0x04)) 
				i = (i & 0x0F) + ((i / 16) * 10);
			// Convert 12 hour clock to 24 hour clock if necessary
			if (eax == RTC_HOURS && !(j & 0x02) && (i & 0x80))
				i = ((i & 0x7F) + 12) % 24;
			reg[7] = i;
			break;
		case 36:
			if (ecx < 0) {
				ecx = 0;
			}
			if (ebx != 0) {
				*((int *) (ebx + ds_base)) = memman_total(memman);
			}
			if (ecx > memman->frees) {
				ecx = memman->frees;
			}
			for (i = 0; i < ecx; i++) {
				*((unsigned int *) (eax + ds_base + i * 8 + 0)) = memman->free[i].addr;
				*((unsigned int *) (eax + ds_base + i * 8 + 4)) = memman->free[i].size;
			}
			reg[7] = ecx;
			break;
		case 37:
			reg[7] = memman_get_algo_id();
			break;

		/* memfs: api70 - api77 */
		case 70: /* int api_memfs_format(int disk_kb); */
			reg[7] = memfs_format(eax);
			break;
		case 71: /* int api_memfs_mkdir(char *path); */
			if (ebx == 0) {
				reg[7] = -1;
			} else {
				reg[7] = memfs_mkdir((char *) ebx + ds_base);
			}
			break;
		case 72: /* int api_memfs_create(char *path); */
			if (ebx == 0) {
				reg[7] = -1;
			} else {
				reg[7] = memfs_create((char *) ebx + ds_base);
			}
			break;
		case 73: /* int api_memfs_write(char *path, int offset, char *buf, int len); */
			if (ebx == 0 || ecx == 0) {
				reg[7] = -1;
			} else {
				reg[7] = memfs_write((char *) ebx + ds_base, eax, (char *) ecx + ds_base, esi);
			}
			break;
		case 74: /* int api_memfs_read(char *path, int offset, char *buf, int len); */
			if (ebx == 0 || ecx == 0) {
				reg[7] = -1;
			} else {
				reg[7] = memfs_read((char *) ebx + ds_base, eax, (char *) ecx + ds_base, esi);
			}
			break;
		case 75: /* int api_memfs_copy(char *src, char *dst); */
			if (ebx == 0 || ecx == 0) {
				reg[7] = -1;
			} else {
				reg[7] = memfs_copy((char *) ebx + ds_base, (char *) ecx + ds_base);
			}
			break;
		case 76: /* int api_memfs_delete(char *path); */
			if (ebx == 0) {
				reg[7] = -1;
			} else {
				reg[7] = memfs_delete((char *) ebx + ds_base);
			}
			break;
		case 77: /* int api_memfs_list(char *path, char *outbuf, int outbuf_len); */
			if (ebx == 0 || ecx == 0) {
				reg[7] = -1;
			} else {
				reg[7] = memfs_list((char *) ebx + ds_base, (char *) ecx + ds_base, eax);
			}
			break;
	}
	return 0;
}

int *inthandler0c(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[40];
	cons_putstr0(cons, "堆栈异常，应用软件程序执行错误！！\nINT 0C :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);
}

int *inthandler0d(int *esp)
{
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[40];
	cons_putstr0(cons, "一般保护例外，应用已停止运行，应用触发保护程序。\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);
}

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col)
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	if (dx < 0) {
		dx = - dx;
	}
	if (dy < 0) {
		dy = - dy;
	}
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		} else {
			dx =  1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		} else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	} else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		} else {
			dy =  1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		} else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (i = 0; i < len; i++) {
		sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}
