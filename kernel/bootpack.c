//============================================//
// Helo OS COPYRIGHT (C) 2019-2020 SYON       //
////////////////////////////////////////////////
// HELO OS BY:STON 2020
// COPYRIGHT (C) 2019-2020 STON
// ����Դ������
// STON/PENGZZEKAI/HELO
//
// =================================
//
// 2357749867@qq.com
////////////////////////////////////////////

/*
 |--|  |--|             |--|
 |  |  |  |             |  |
 |  |--|  |    _____    |  |    ______
 |        |   //---\\   |  |   //---\ \
 |  |--|  |  | |___| |  |  |   ||   | |
 |  |  |  |  | |___/-\  |  |   ||   | |
 |--|  |--|   \_____/   |--|   \\___/ /
*/
#include "bootpack.h"
#include <stdio.h>

#define KEYCMD_LED 0xed

/* Scheduler validation profiles (compile-time switch).
	0: default runtime mix
	1: CPU-bound demotion test
	2: interactive responsiveness test
	3: starvation/aging test
	4: CPU-bound demotion test (aging off)
*/
#define SCHED_TEST_DEFAULT 0
#define SCHED_TEST_CPU_BOUND 1
#define SCHED_TEST_INTERACTIVE 2
#define SCHED_TEST_AGING 3
#define SCHED_TEST_CPU_BOUND_NO_AGING 4

#ifndef SCHED_TEST_PROFILE
#define SCHED_TEST_PROFILE SCHED_TEST_CPU_BOUND_NO_AGING
#endif

#if SCHED_TEST_PROFILE == SCHED_TEST_CPU_BOUND
#define TEST_HOG_COUNT 1
#define TEST_HOG_LEVEL 0
#define TEST_CREATE_IO_TASK 1
#define TEST_IO_LEVEL 2
#define TEST_CREATE_LOW_HOG 0
#define TEST_LOW_HOG_LEVEL (MAX_TASKLEVELS - 2)
#define TEST_ENABLE_AGING 1
#elif SCHED_TEST_PROFILE == SCHED_TEST_INTERACTIVE
#define TEST_HOG_COUNT 1
#define TEST_HOG_LEVEL 2
#define TEST_CREATE_IO_TASK 1
#define TEST_IO_LEVEL 0
#define TEST_CREATE_LOW_HOG 0
#define TEST_LOW_HOG_LEVEL (MAX_TASKLEVELS - 2)
#define TEST_ENABLE_AGING 1
#elif SCHED_TEST_PROFILE == SCHED_TEST_AGING
#define TEST_HOG_COUNT 5
#define TEST_HOG_LEVEL 0
#define TEST_CREATE_IO_TASK 1
#define TEST_IO_LEVEL 2
#define TEST_CREATE_LOW_HOG 1
#define TEST_LOW_HOG_LEVEL (MAX_TASKLEVELS - 2)
#define TEST_ENABLE_AGING 1
#elif SCHED_TEST_PROFILE == SCHED_TEST_CPU_BOUND_NO_AGING
#define TEST_HOG_COUNT 1
#define TEST_HOG_LEVEL 0
#define TEST_CREATE_IO_TASK 1
#define TEST_IO_LEVEL 2
#define TEST_CREATE_LOW_HOG 0
#define TEST_LOW_HOG_LEVEL (MAX_TASKLEVELS - 2)
#define TEST_ENABLE_AGING 0
#elif SCHED_TEST_PROFILE == SCHED_TEST_DEFAULT
#define TEST_HOG_COUNT 5
#define TEST_HOG_LEVEL 2
#define TEST_CREATE_IO_TASK 1
#define TEST_IO_LEVEL 2
#define TEST_CREATE_LOW_HOG 1
#define TEST_LOW_HOG_LEVEL (MAX_TASKLEVELS - 2)
#define TEST_ENABLE_AGING 1
#else
#error "Invalid SCHED_TEST_PROFILE"
#endif

int g_sched_enable_aging = TEST_ENABLE_AGING;

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
void close_console(struct SHEET *sht);
void close_constask(struct TASK *task);

static void task_mon_refresh(struct SHEET *sht)
{
	unsigned int now_tick = timerctl.count;
	struct TASK *now_task = task_now();
	int i, y;
	char s[96];

	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, 3, 24, sht->bxsize - 4, sht->bysize - 4);
	sprintf(s, "MLFQ  AGING=%s  now_lv=%02d  tick=%u",
					g_sched_enable_aging ? "ON" : "OFF", taskctl->now_lv, now_tick);
	putfonts8_asc_sht(sht, 10, 30, COL8_000000, COL8_C6C6C6, s, 34);

	sprintf(s, "Q: L0=%02d L1=%02d L2=%02d L3=%02d L4=%02d",
					taskctl->level[0].running, taskctl->level[1].running, taskctl->level[2].running,
					taskctl->level[3].running, taskctl->level[4].running);
	putfonts8_asc_sht(sht, 10, 46, COL8_000000, COL8_C6C6C6, s, 40);
	sprintf(s, "   L5=%02d L6=%02d L7=%02d L8=%02d L9=%02d",
					taskctl->level[5].running, taskctl->level[6].running, taskctl->level[7].running,
					taskctl->level[8].running, taskctl->level[9].running);
	putfonts8_asc_sht(sht, 10, 62, COL8_000000, COL8_C6C6C6, s, 40);

	putfonts8_asc_sht(sht, 10, 84, COL8_000000, COL8_C6C6C6,
										"ID LV P F ENQ      WAIT     AGE_LIM RUN", 38);

	y = 100;
	for (i = 0; i < MAX_TASKS; i++)
	{
		struct TASK *t = &taskctl->tasks0[i];
		unsigned int wait_tick;
		if (t->flags == 0)
		{
			continue;
		}
		if (t == now_task)
		{
			wait_tick = 0;
		}
		else
		{
			wait_tick = now_tick - t->enqueue_tick;
		}
		sprintf(s, "%03d %02d %02d %d %8u %8u %8d   %c",
						i, t->level, t->priority, t->flags, t->enqueue_tick,
						wait_tick, task_aging_limit_for_level(t->level), (t == now_task) ? '*' : ' ');
		putfonts8_asc_sht(sht, 10, y, COL8_000000, COL8_C6C6C6, s, 46);
		y += 16;
		if (y >= sht->bysize - 16)
		{
			break;
		}
	}
	sheet_refresh(sht, 3, 24, sht->bxsize - 3, sht->bysize - 3);
	return;
}

void task_mon(struct SHEET *sht)
{
	struct TASK *task = task_now();
	struct TIMER *timer;
	struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
	struct FIFO32 *sys_fifo = (struct FIFO32 *)*((int *)0x0fec);
	int i;

	timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);
	task_mon_refresh(sht);

	for (;;)
	{
		io_cli();
		if (fifo32_status(&task->fifo) == 0)
		{
			task_sleep(task);
			io_sti();
		}
		else
		{
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i == 1)
			{
				task_mon_refresh(sht);
				timer_settime(timer, 50);
			}
			else if (i == 4)
			{
				timer_cancel(timer);
				io_cli();
				fifo32_put(sys_fifo, sht - shtctl->sheets0 + 2024);
				io_sti();
				task_sleep(task);
			}
		}
	}
}

void task_hog(void)
{
	for (;;)
	{
	}
}

/* Interactive task */
void task_interactive(void)
{
	struct TASK *task = task_now();
	struct TIMER *timer;
	int fifobuf[128];

	fifo32_init(&task->fifo, 128, fifobuf, task);
	timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 10);

	for (;;)
	{
		io_cli();
		if (fifo32_status(&task->fifo) == 0)
		{
			task_sleep(task);
			io_sti();
		}
		else
		{
			fifo32_get(&task->fifo);
			io_sti();
			timer_settime(timer, 10);
		}
	}
}

void _main()
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	struct SHTCTL *shtctl;
	char s[40];
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	int mx, my, i, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
	unsigned int memtotal;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	unsigned char *buf_back, buf_mouse[256];
	struct SHEET *sht_back, *sht_mouse;
	struct TASK *task_a, *task;
	// ���ļ���ӳ��
	/*static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};*/
	// ���ļ���ӳ��
	static char keytable0[0x80] = {
			0,
			0,
			'1',
			'2',
			'3',
			'4',
			'5',
			'6',
			'7',
			'8',
			'9',
			'0',
			'-',
			'=',
			0x08,
			0,
			'Q',
			'W',
			'E',
			'R',
			'T',
			'Y',
			'U',
			'I',
			'O',
			'P',
			'[',
			']',
			0x0a,
			0,
			'A',
			'S',
			'D',
			'F',
			'G',
			'H',
			'J',
			'K',
			'L',
			';',
			'\'',
			'`',
			0,
			'\\',
			'Z',
			'X',
			'C',
			'V',
			'B',
			'N',
			'M',
			',',
			'.',
			'/',
			0,
			'*',
			0,
			' ',
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			'7',
			0,
			'9',
			'-',
			'4',
			'5',
			'6',
			'+',
			'1',
			0,
			'3',
			'0',
			'.',
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0x5c,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0x5c,
			0,
			0,
	};
	static char keytable1[0x80] = {
			0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x08, 0,
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0x0a, 0, 'A', 'S',
			'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
			'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
			'2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, '_', 0, 0, 0, 0, 0, 0, 0, 0, 0, '|', 0, 0};
	int key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;
	struct SHEET *sht = 0, *key_win, *sht2;
	int *fat;
	unsigned char *nihongo;
	struct FILEINFO *finfo;
	extern char hankaku[4096];

	init_gdtidt();
	init_pic();
	io_sti();
	fifo32_init(&fifo, 128, fifobuf, 0);
	*((int *)0x0fec) = (int)&fifo;
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8);
	io_out8(PIC1_IMR, 0xef);
	fifo32_init(&keycmd, 32, keycmd_buf, 0);

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 0);
	*((int *)0x0fe4) = (int)shtctl;
	task_a->langmode = 0;

	/* sht_back */
	sht_back = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* �����F�Ȃ� */
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);

	/* sht_cons */
	key_win = open_console(shtctl, memtotal);

	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* ��ʒ����ɂȂ�悤�ɍ��W�v�Z */
	my = (binfo->scrny - 28 - 16) / 2;

	sheet_slide(sht_back, 0, 0);
	sheet_slide(key_win, 32, 4);
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back, 0);
	sheet_updown(key_win, 1);
	sheet_updown(sht_mouse, 2);
	keywin_on(key_win);

	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);

	//-------------------------
	// ZIKU HZK16.FNT
	// nihongo = (unsigned char *) memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
	nihongo = (unsigned char *)memman_alloc_4k(memman, 0x5d5d * 32);
	fat = (int *)memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));
	finfo = file_search("HZK16.fnt", (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
	if (finfo != 0)
	{
		file_loadfile(finfo->clustno, finfo->size, nihongo, fat, (char *)(ADR_DISKIMG + 0x003e00));
	}
	else
	{
		for (i = 0; i < 16 * 256; i++)
		{
			nihongo[i] = hankaku[i]; /* �t�H���g���Ȃ������̂Ŕ��p�������R�s�[ */
		}
		for (i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++)
		{
			nihongo[i] = 0xff; /* �t�H���g���Ȃ������̂őS�p������0xff�Ŗ��ߐs���� */
		}
	}

	*((int *)0x0fe8) = (int)nihongo;
	memman_free_4k(memman, (int)fat, 4 * 2880);

	struct TASK *task_hog_t;
	for (i = 0; i < TEST_HOG_COUNT; i++)
	{
		task_hog_t = task_alloc();
		task_hog_t->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
		task_hog_t->tss.eip = (int)&task_hog;
		task_hog_t->tss.es = 1 * 8;
		task_hog_t->tss.cs = 2 * 8;
		task_hog_t->tss.ss = 1 * 8;
		task_hog_t->tss.ds = 1 * 8;
		task_hog_t->tss.fs = 1 * 8;
		task_hog_t->tss.gs = 1 * 8;
		task_run(task_hog_t, TEST_HOG_LEVEL, 0);
	}

#if TEST_CREATE_IO_TASK
	{
		struct TASK *task_io_t = task_alloc();
		task_io_t->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
		task_io_t->tss.eip = (int)&task_interactive;
		task_io_t->tss.es = 1 * 8;
		task_io_t->tss.cs = 2 * 8;
		task_io_t->tss.ss = 1 * 8;
		task_io_t->tss.ds = 1 * 8;
		task_io_t->tss.fs = 1 * 8;
		task_io_t->tss.gs = 1 * 8;
		task_run(task_io_t, TEST_IO_LEVEL, 0);
	}
#endif

#if TEST_CREATE_LOW_HOG
	{
		struct TASK *task_hog_low = task_alloc();
		task_hog_low->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
		task_hog_low->tss.eip = (int)&task_hog;
		task_hog_low->tss.es = 1 * 8;
		task_hog_low->tss.cs = 2 * 8;
		task_hog_low->tss.ss = 1 * 8;
		task_hog_low->tss.ds = 1 * 8;
		task_hog_low->tss.fs = 1 * 8;
		task_hog_low->tss.gs = 1 * 8;
		task_run(task_hog_low, TEST_LOW_HOG_LEVEL, 0);
	}
#endif

	for (;;)
	{
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0)
		{
			/* �L�[�{�[�h�R���g���[���ɑ���f�[�^������΁A���� */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0)
		{
			/* FIFO��������ۂɂȂ����̂ŁA�ۗ����Ă���`�悪����Ύ��s���� */
			if (new_mx >= 0)
			{
				io_sti();
				sheet_slide(sht_mouse, new_mx, new_my);
				new_mx = -1;
			}
			else if (new_wx != 0x7fffffff)
			{
				io_sti();
				sheet_slide(sht, new_wx, new_wy);
				new_wx = 0x7fffffff;
			}
			else
			{
				task_sleep(task_a);
				io_sti();
			}
		}
		else
		{
			i = fifo32_get(&fifo);
			io_sti();
			if (key_win != 0 && key_win->flags == 0)
			{ /* �E�B���h�E������ꂽ */
				if (shtctl->top == 1)
				{ /* �����}�E�X�Ɣw�i�����Ȃ� */
					key_win = 0;
				}
				else
				{
					key_win = shtctl->sheets[shtctl->top - 1];
					keywin_on(key_win);
				}
			}
			if (256 <= i && i <= 511)
			{ /* �L�[�{�[�h�f�[�^ */
				if (i < 0x80 + 256)
				{ /* �L�[�R�[�h�𕶎��R�[�h�ɕϊ� */
					if (key_shift == 0)
					{
						s[0] = keytable0[i - 256];
					}
					else
					{
						s[0] = keytable1[i - 256];
					}
				}
				else
				{
					s[0] = 0;
				}
				if ('A' <= s[0] && s[0] <= 'Z')
				{ /* ���͕������A���t�@�x�b�g */
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0))
					{
						s[0] += 0x20; /* �啶�����������ɕϊ� */
					}
				}
				if (s[0] != 0 && key_win != 0)
				{ /* �ʏ핶���A�o�b�N�X�y�[�X�AEnter */
					fifo32_put(&key_win->task->fifo, s[0] + 256);
				}
				if (i == 256 + 0x0f && key_win != 0)
				{ /* Tab */
					keywin_off(key_win);
					j = key_win->height - 1;
					if (j == 0)
					{
						j = shtctl->top - 1;
					}
					key_win = shtctl->sheets[j];
					keywin_on(key_win);
				}
				if (i == 256 + 0x2a)
				{ /* ���V�t�g ON */
					key_shift |= 1;
				}
				if (i == 256 + 0x36)
				{ /* �E�V�t�g ON */
					key_shift |= 2;
				}
				if (i == 256 + 0xaa)
				{ /* ���V�t�g OFF */
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6)
				{ /* �E�V�t�g OFF */
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a)
				{ /* CapsLock */
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45)
				{ /* NumLock */
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46)
				{ /* ScrollLock */
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x3b && key_shift != 0 && key_win != 0)
				{ /* Shift+F1 */
					task = key_win->task;
					if (task != 0 && task->tss.ss0 != 0)
					{
						cons_putstr0(task->cons, "");
						io_cli();
						task->tss.eax = (int)&(task->tss.esp0);
						task->tss.eip = (int)asm_end_app;
						io_sti();
						task_run(task, -1, 0);
					}
				}
				if (i == 256 + 0x3c && key_shift != 0)
				{ /* Shift+F2 */
					if (key_win != 0)
					{
						keywin_off(key_win);
					}
					key_win = open_console(shtctl, memtotal);
					sheet_slide(key_win, 32, 4);
					sheet_updown(key_win, shtctl->top);
					keywin_on(key_win);
				}
				if (i == 256 + 0x57)
				{ /* F11 */
					sheet_updown(shtctl->sheets[1], shtctl->top - 1);
				}
				if (i == 256 + 0xfa)
				{
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe)
				{
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
			}
			else if (512 <= i && i <= 767)
			{
				if (mouse_decode(&mdec, i - 512) != 0)
				{
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0)
					{
						mx = 0;
					}
					if (my < 0)
					{
						my = 0;
					}
					if (mx > binfo->scrnx - 1)
					{
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1)
					{
						my = binfo->scrny - 1;
					}
					new_mx = mx;
					new_my = my;
					if ((mdec.btn & 0x01) != 0)
					{
						if (mmx < 0)
						{
							for (j = shtctl->top - 1; j > 0; j--)
							{
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize)
								{
									{
										sheet_updown(sht, shtctl->top - 1);
										if (sht != key_win)
										{
											keywin_off(key_win);
											key_win = sht;
											keywin_on(key_win);
										}
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21)
										{
											mmx = mx;
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19)
										{
											if ((sht->flags & 0x10) != 0)
											{
												task = sht->task;
												cons_putstr0(task->cons, "");
												io_cli();
												task->tss.eax = (int)&(task->tss.esp0);
												task->tss.eip = (int)asm_end_app;
												io_sti();
												task_run(task, -1, 0);
											}
											else
											{
												task = sht->task;
												sheet_updown(sht, -1);
												keywin_off(key_win);
												key_win = shtctl->sheets[shtctl->top - 1];
												keywin_on(key_win);
												io_cli();
												fifo32_put(&task->fifo, 4);
												io_sti();
											}
										}
										break;
									}
								}
							}
						}
						else
						{
							x = mx - mmx;
							y = my - mmy;
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = my;
						}
					}
					else
					{
						mmx = -1; /* �ʏ탂�[�h�� */
						if (new_wx != 0x7fffffff)
						{
							sheet_slide(sht, new_wx, new_wy); /* ��x�m�肳���� */
							new_wx = 0x7fffffff;
						}
					}
				}
			}
			else if (768 <= i && i <= 1023)
			{
				close_console(shtctl->sheets0 + (i - 768));
			}
			else if (1024 <= i && i <= 2023)
			{
				close_constask(taskctl->tasks0 + (i - 1024));
			}
			else if (2024 <= i && i <= 2279)
			{
				sht2 = shtctl->sheets0 + (i - 2024);
				if (sht2->task != 0)
				{
					close_constask(sht2->task);
				}
				memman_free_4k(memman, (int)sht2->buf, sht2->bxsize * sht2->bysize);
				sheet_free(sht2);
			}
			/* -------------------------------------------------------------------------------------------------- */
			// ��ʾ����
			sprintf(s, "DATE: %d-%d-%d", get_year(), get_mon_hex(), get_day_of_month());
			putfonts8_asc_sht(sht_back, binfo->scrnx - 180, binfo->scrny - 20, COL8_000000, COL8_C6C6C6, s, 15);
			// ��ʾʱ��
			sprintf(s, "%d:%d", get_hour_hex(), get_min_hex());
			putfonts8_asc_sht(sht_back, binfo->scrnx - 45, binfo->scrny - 20, COL8_000000, COL8_C6C6C6, s, 5);
			sheet_refresh(sht_back, binfo->scrnx - 130, binfo->scrny - 20, binfo->scrnx - 45 + 5 * 8, binfo->scrny - 50 + 16);
			/* -------------------------------------------------------------------------------------------------- */
		}
	}
}

void keywin_off(struct SHEET *key_win)
{
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0)
	{
		fifo32_put(&key_win->task->fifo, 3);
	}
	return;
}

void keywin_on(struct SHEET *key_win)
{
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0)
	{
		fifo32_put(&key_win->task->fifo, 2);
	}
	return;
}

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	struct TASK *task = task_alloc();
	int *cons_fifo = (int *)memman_alloc_4k(memman, 128 * 4);
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int)&console_task;
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	*((int *)(task->tss.esp + 4)) = (int)sht;
	*((int *)(task->tss.esp + 8)) = memtotal;
	task_run(task, 2, 0);
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	return task;
}

/* ------------------------------
���ڴ�С�ĸ��� ---- ��Ҫ����
--------------------------------- */
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	struct SHEET *sht = sheet_alloc(shtctl);
	unsigned char *buf = (unsigned char *)memman_alloc_4k(memman, 525 * 479);
	sheet_setbuf(sht, buf, 525, 479, 255);
	make_window8(buf, 525, 479, "Helo OS CONSOLE", 0);
	make_textbox8(sht, 3, 24, 519, 452, COL8_FFFFFF);
	sht->task = open_constask(sht, memtotal);
	sht->flags |= 0x20;
	return sht;
}

struct SHEET *open_taskmon(struct SHTCTL *shtctl, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	struct SHEET *sht = sheet_alloc(shtctl);
	struct TASK *task = task_alloc();
	int *mon_fifo = (int *)memman_alloc_4k(memman, 525 * 4);
	unsigned char *buf = (unsigned char *)memman_alloc_4k(memman, 520 * 420);

	sheet_setbuf(sht, buf, 520, 420, -1);
	make_window8(buf, 520, 420, "TASK MONITOR (MLFQ+AGING)", 0);
	sheet_slide(sht, ((shtctl->xsize - 520) / 2) & ~3, (shtctl->ysize - 420) / 2);
	sheet_updown(sht, shtctl->top);

	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int)&task_mon;
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	task->langmode = 0;
	task->langbyte1 = 0;
	*((int *)(task->tss.esp + 4)) = (int)sht;
	fifo32_init(&task->fifo, 128, mon_fifo, task);
	task_run(task, 1, 0);
	sht->task = task;
	(void)memtotal;
	return sht;
}

void close_constask(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int)task->fifo.buf, 525 * 4);
	task->flags = 0;
	return;
}

void close_console(struct SHEET *sht)
{
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	struct TASK *task = sht->task;
	memman_free_4k(memman, (int)sht->buf, 770 * 655);
	sheet_free(sht);
	close_constask(task);
	return;
}
