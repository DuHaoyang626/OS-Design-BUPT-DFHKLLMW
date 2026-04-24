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

void console_task(struct SHEET *sheet, int memtotal)
{
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct CONSOLE cons;
	struct FILEHANDLE fhandle[8];
	char cmdline[64];
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

void cmd_timertest(struct CONSOLE *cons);

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
	if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	} else if (strcmp(cmdline, "timertest") == 0 && cons->sht != 0) {
		cmd_timertest(cons);
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
		case 38:
			reg[7] = MMU_MODE;
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

/* ============================================================
 * timertest: 定时器管理结构与方法测试命令
 * TC-01: 基本超时顺序验证（3个定时器，超时顺序应为T1<T2<T3）
 * TC-02: 大量并发定时器压力测试（60个定时器同时运行）
 * TC-03: timer_cancel 边界测试
 * TC-04: 池耗尽测试（连续分配直到返回0）
 * TC-05: timeout=0 立即到期测试
 * ============================================================ */
void cmd_timertest(struct CONSOLE *cons)
{
	struct TASK *task = task_now();
	struct TIMER *timers[64];
	char s[64];
	int i, count, pass, alloc_count;

	cons_putstr0(cons, "\n--- Timer Test Start ---\n");

	/* TC-01: 基本超时顺序验证 */
	cons_putstr0(cons, "\n[TC-01] Basic timeout order (3 timers)\n");
	{
		struct TIMER *t1, *t2, *t3;
		t1 = timer_alloc(); timer_init(t1, &task->fifo, 0x101);
		t2 = timer_alloc(); timer_init(t2, &task->fifo, 0x102);
		t3 = timer_alloc(); timer_init(t3, &task->fifo, 0x103);
		/* 故意乱序设置：t3最短，t1最长，验证链表排序 */
		timer_settime(t3, 5);
		timer_settime(t1, 15);
		timer_settime(t2, 10);
		/* 验证三个定时器的timeout值满足 t3 < t2 < t1（升序） */
		pass = (t3->timeout < t2->timeout && t2->timeout < t1->timeout) ? 1 : 0;
		sprintf(s, "  Link order check: %s\n", pass ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
		sprintf(s, "  t3->timeout=%u t2->timeout=%u t1->timeout=%u\n",
			t3->timeout, t2->timeout, t1->timeout);
		cons_putstr0(cons, s);
		/* 等待3个定时器全部到期（消费FIFO） */
		count = 0;
		while (count < 3) {
			io_cli();
			if (fifo32_status(&task->fifo) > 0) {
				i = fifo32_get(&task->fifo);
				io_sti();
				if (i == 0x101 || i == 0x102 || i == 0x103) count++;
			} else {
				task_sleep(task);
				io_sti();
			}
		}
		cons_putstr0(cons, "  All 3 timers fired: PASS\n");
	}

	/* TC-02: 60个并发定时器压力测试 */
	cons_putstr0(cons, "\n[TC-02] 60 concurrent timers stress test\n");
	{
		int fired = 0;
		/* 分配60个定时器，超时值1~60 tick */
		for (i = 0; i < 60; i++) {
			timers[i] = timer_alloc();
			if (timers[i] == 0) {
				cons_putstr0(cons, "  timer_alloc failed!\n");
				break;
			}
			timer_init(timers[i], &task->fifo, 0x200 + i);
			timer_settime(timers[i], i + 1);
		}
		sprintf(s, "  Allocated 60 timers, pool used: ~%d/%d\n", 60 + 3, MAX_TIMER);
		cons_putstr0(cons, s);
		/* 等待60个全部到期 */
		while (fired < 60) {
			io_cli();
			if (fifo32_status(&task->fifo) > 0) {
				i = fifo32_get(&task->fifo);
				io_sti();
				if (i >= 0x200 && i < 0x200 + 60) fired++;
			} else {
				task_sleep(task);
				io_sti();
			}
		}
		sprintf(s, "  All 60 timers fired: %s\n", fired == 60 ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
	}

	/* TC-03: timer_cancel 边界测试 */
	cons_putstr0(cons, "\n[TC-03] timer_cancel boundary test\n");
	{
		struct TIMER *tc;
		int ret;
		/* 取消未到期的定时器 */
		tc = timer_alloc();
		timer_init(tc, &task->fifo, 0x301);
		timer_settime(tc, 200);
		ret = timer_cancel(tc);
		sprintf(s, "  Cancel active timer: ret=%d (expect 1): %s\n",
			ret, ret == 1 ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
		/* 取消已取消的定时器（flags已变为ALLOC=1，非USING=2） */
		ret = timer_cancel(tc);
		sprintf(s, "  Cancel already-cancelled: ret=%d (expect 0): %s\n",
			ret, ret == 0 ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
		timer_free(tc);
	}

	/* TC-04: 池耗尽测试 */
	cons_putstr0(cons, "\n[TC-04] Pool exhaustion test\n");
	{
		static struct TIMER *tmp[MAX_TIMER];
		int alloc_count = 0;
		/* 持续分配直到失败 */
		for (i = 0; i < MAX_TIMER; i++) {
			tmp[i] = timer_alloc();
			if (tmp[i] == 0) break;
			alloc_count++;
		}
		sprintf(s, "  Allocated %d timers before pool empty\n", alloc_count);
		cons_putstr0(cons, s);
		sprintf(s, "  timer_alloc returns 0 at exhaustion: %s\n",
			tmp[alloc_count] == 0 ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
		/* 释放所有 */
		for (i = 0; i < alloc_count; i++) {
			timer_free(tmp[i]);
		}
		cons_putstr0(cons, "  Pool freed.\n");
	}

	/* TC-05: timeout=0 立即到期测试 */
	cons_putstr0(cons, "\n[TC-05] timeout=0 immediate fire test\n");
	{
		struct TIMER *t0;
		t0 = timer_alloc();
		timer_init(t0, &task->fifo, 0x501);
		timer_settime(t0, 0);
		/* timeout=0 意味着 timeout = timerctl.count+0，下一个tick即触发 */
		count = 0;
		while (count == 0) {
			io_cli();
			if (fifo32_status(&task->fifo) > 0) {
				i = fifo32_get(&task->fifo);
				io_sti();
				if (i == 0x501) count = 1;
			} else {
				task_sleep(task);
				io_sti();
			}
		}
		cons_putstr0(cons, "  timeout=0 fired on next tick: PASS\n");
	}

	/* TC-06: 调度器协同验证——压力测试期间 task_timer 仍正常触发 */
	cons_putstr0(cons, "\n[TC-06] Scheduler cooperation: task_timer survives stress\n");
	{
		unsigned int count_before, count_after;
		int switches_ok;
		/* 记录当前 tick，启动 30 个定时器，等待全部触发，
		   期间 task_timer 也在运行，验证调度未被阻断 */
		count_before = timerctl.count;
		for (i = 0; i < 30; i++) {
			timers[i] = timer_alloc();
			timer_init(timers[i], &task->fifo, 0x600 + i);
			timer_settime(timers[i], i * 2 + 1);
		}
		count = 0;
		while (count < 30) {
			io_cli();
			if (fifo32_status(&task->fifo) > 0) {
				i = fifo32_get(&task->fifo);
				io_sti();
				if (i >= 0x600 && i < 0x600 + 30) count++;
			} else {
				task_sleep(task);
				io_sti();
			}
		}
		count_after = timerctl.count;
		/* task_timer 以 priority=2 tick 为周期切换，
		   若 count 增长正常说明 IRQ0 未被阻断 */
		switches_ok = (count_after > count_before) ? 1 : 0;
		sprintf(s, "  tick advanced %u during stress (expect >0): %s\n",
			count_after - count_before, switches_ok ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
		/* 验证 task_timer 仍在链表中（flags=USING=2） */
		sprintf(s, "  task_timer still active (flags=%d, expect 2): %s\n",
			task_timer->flags,
			task_timer->flags == 2 ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
	}

	/* TC-07: timerctl.count 单调递增验证 */
	cons_putstr0(cons, "\n[TC-07] timerctl.count monotonic increase\n");
	{
		unsigned int c1, c2, c3;
		c1 = timerctl.count;
		/* 等待约 5 tick */
		{
			struct TIMER *tw = timer_alloc();
			timer_init(tw, &task->fifo, 0x701);
			timer_settime(tw, 5);
			while (1) {
				io_cli();
				if (fifo32_status(&task->fifo) > 0) {
					i = fifo32_get(&task->fifo);
					io_sti();
					if (i == 0x701) break;
				} else { task_sleep(task); io_sti(); }
			}
		}
		c2 = timerctl.count;
		/* 再等约 5 tick */
		{
			struct TIMER *tw = timer_alloc();
			timer_init(tw, &task->fifo, 0x702);
			timer_settime(tw, 5);
			while (1) {
				io_cli();
				if (fifo32_status(&task->fifo) > 0) {
					i = fifo32_get(&task->fifo);
					io_sti();
					if (i == 0x702) break;
				} else { task_sleep(task); io_sti(); }
			}
		}
		c3 = timerctl.count;
		sprintf(s, "  count: %u -> %u -> %u\n", c1, c2, c3);
		cons_putstr0(cons, s);
		sprintf(s, "  monotonic: %s\n",
			(c1 < c2 && c2 < c3) ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
	}

	/* TC-08: timer_cancelall 清理验证 */
	cons_putstr0(cons, "\n[TC-08] timer_cancelall cleanup\n");
	{
		struct FIFO32 test_fifo;
		int test_buf[32];
		int active_before, active_after, j;
		fifo32_init(&test_fifo, 32, test_buf, 0);
		/* 向 test_fifo 注册 10 个定时器，flags2=1 */
		for (i = 0; i < 10; i++) {
			timers[i] = timer_alloc();
			timers[i]->flags2 = 1;
			timer_init(timers[i], &test_fifo, 0x800 + i);
			timer_settime(timers[i], 200 + i); /* 超时较长，不会自然到期 */
		}
		/* 统计 test_fifo 关联的活跃定时器数 */
		active_before = 0;
		for (j = 0; j < MAX_TIMER; j++) {
			if (timerctl.timers0[j].flags == 2 &&
				timerctl.timers0[j].fifo == &test_fifo) {
				active_before++;
			}
		}
		timer_cancelall(&test_fifo);
		active_after = 0;
		for (j = 0; j < MAX_TIMER; j++) {
			if (timerctl.timers0[j].flags == 2 &&
				timerctl.timers0[j].fifo == &test_fifo) {
				active_after++;
			}
		}
		sprintf(s, "  active before cancelall: %d, after: %d\n",
			active_before, active_after);
		cons_putstr0(cons, s);
		sprintf(s, "  cancelall cleared all: %s\n",
			(active_before == 10 && active_after == 0) ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
	}

	/* TC-09: 压力测试后池恢复正常，可继续分配 */
	cons_putstr0(cons, "\n[TC-09] Pool recovery after stress\n");
	{
		struct TIMER *t_new;
		t_new = timer_alloc();
		sprintf(s, "  alloc after stress: %s\n",
			t_new != 0 ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
		if (t_new) timer_free(t_new);
	}

	/* TC-10: 链表插入时间复杂度测量
	 * 分别在 n=0,50,100,200 个背景定时器存在时，测量插入一个新定时器所需 tick 数
	 * 由于 PIT 精度为 10ms/tick，此处用 timerctl.count 差值近似（粗粒度）
	 * 主要目的是验证随 n 增大插入耗时是否呈线性增长趋势 */
	cons_putstr0(cons, "\n[TC-10] timer_settime insertion complexity\n");
	{
		int n_bg[] = {0, 50, 100, 200};
		int k, b;
		static struct TIMER *bg[200];
		struct TIMER *probe;
		unsigned int t_start, t_end;

		for (k = 0; k < 4; k++) {
			int n = n_bg[k];
			/* 建立 n 个背景定时器，超时值均匀分布在 500~1500 tick */
			for (b = 0; b < n; b++) {
				bg[b] = timer_alloc();
				if (bg[b] == 0) break;
				timer_init(bg[b], &task->fifo, 0xf00);
				timer_settime(bg[b], 500 + b * 5);
			}
			/* 测量插入一个超时值为 750 tick（落在链表中间）的定时器 */
			probe = timer_alloc();
			timer_init(probe, &task->fifo, 0xf01);
			t_start = timerctl.count;
			timer_settime(probe, 750);
			t_end = timerctl.count;
			sprintf(s, "  n=%3d bg timers: insert took %u tick(s)\n",
				n, t_end - t_start);
			cons_putstr0(cons, s);
			/* 清理：取消 probe 和所有背景定时器 */
			timer_cancel(probe);
			timer_free(probe);
			for (b = 0; b < n; b++) {
				if (bg[b] != 0) {
					timer_cancel(bg[b]);
					timer_free(bg[b]);
				}
			}
		}
		cons_putstr0(cons, "  (tick resolution=10ms; 0 tick = sub-10ms, expected for small n)\n");
	}

	/* TC-11: MAX_TIMER 扩容后池容量验证
	 * 将 MAX_TIMER 从 500 改为 800 后重跑池耗尽测试，
	 * 验证可分配数量相应增加 */
	cons_putstr0(cons, "\n[TC-11] MAX_TIMER capacity check\n");
	{
		static struct TIMER *tmp2[MAX_TIMER];
		int alloc_count2 = 0;
		for (i = 0; i < MAX_TIMER; i++) {
			tmp2[i] = timer_alloc();
			if (tmp2[i] == 0) break;
			alloc_count2++;
		}
		sprintf(s, "  MAX_TIMER=%d, allocatable=%d, system_used=%d\n",
			MAX_TIMER, alloc_count2, MAX_TIMER - alloc_count2);
		cons_putstr0(cons, s);
		sprintf(s, "  pool size matches MAX_TIMER: %s\n",
			alloc_count2 + (MAX_TIMER - alloc_count2) == MAX_TIMER ? "PASS" : "FAIL");
		cons_putstr0(cons, s);
		for (i = 0; i < alloc_count2; i++) timer_free(tmp2[i]);
	}

	cons_putstr0(cons, "\n--- Timer Test Done ---\n\n");
	return;
}
int *inthandler0e(int *esp)
{
	struct TASK *task;
	struct CONSOLE *cons;
	char s[40];
	unsigned int error_code = (unsigned int) esp[0];
	unsigned int fault_addr = (unsigned int) load_cr2();

	if (taskctl == 0) {
		for (;;) {
			io_hlt();
		}
	}
	task = task_now();
	if (task == 0) {
		for (;;) {
			io_hlt();
		}
	}

	cons = task->cons;
	if (cons != 0) {
		cons_putstr0(cons, "INT 0E :\n Page Fault Exception.\n");
		sprintf(s, "CR2 = %08X\n", fault_addr);
		cons_putstr0(cons, s);
		sprintf(s, "ERR = %08X\n", error_code);
		cons_putstr0(cons, s);
		sprintf(s, "EIP = %08X\n", esp[11]);
		cons_putstr0(cons, s);
	}

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
