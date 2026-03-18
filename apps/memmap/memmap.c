#include <stdio.h>
#include <string.h>
#include "apilib.h"

#define TOTAL_MEM_BYTES  (128 * 1024 * 1024U)
#define UNIT_BYTES       (4 * 1024U)
#define PAGE_COUNT       (TOTAL_MEM_BYTES / UNIT_BYTES)
#define MAX_FREE_ENTRIES 4090

#define MAP_COLS 256
#define MAP_ROWS (PAGE_COUNT / MAP_COLS)
#define SCALE    2

#define WIN_W 760
#define WIN_H 360

#define MAP_X 20
#define MAP_Y 52
#define MAP_W (MAP_COLS * SCALE)
#define MAP_H (MAP_ROWS * SCALE)

static void draw_map(char *winbuf, unsigned char *state)
{
	int i, x0, y0, x, y, col;

	for (i = 0; i < PAGE_COUNT; i++) {
		x0 = MAP_X + (i % MAP_COLS) * SCALE;
		y0 = MAP_Y + (i / MAP_COLS) * SCALE;
		col = state[i] ? 0 : 7;
		for (y = 0; y < SCALE; y++) {
			for (x = 0; x < SCALE; x++) {
				winbuf[(y0 + y) * WIN_W + (x0 + x)] = col;
			}
		}
	}
}

int _main(void)
{
	char *winbuf;
	unsigned char *state;
	struct MEMMAP_ENTRY *entries;
	int win, timer, key;
	int i, j;
	int entry_count;
	int free_bytes;
	int free_pages;
	int alloc_pages;
	unsigned int start_page;
	unsigned int end_page;
	char s[64];

	api_initmalloc();
	winbuf = api_malloc(WIN_W * WIN_H);
	state = (unsigned char *) api_malloc(PAGE_COUNT);
	entries = (struct MEMMAP_ENTRY *) api_malloc(MAX_FREE_ENTRIES * sizeof(struct MEMMAP_ENTRY));
	if (winbuf == 0 || state == 0 || entries == 0) {
		api_putstr0("memmap: not enough memory\n");
		api_end();
	}

	win = api_openwin(winbuf, WIN_W, WIN_H, 255, "memmap");
	api_boxfilwin(win, MAP_X - 2, MAP_Y - 2, MAP_X + MAP_W + 1, MAP_Y + MAP_H + 1, 8);

	api_boxfilwin(win, 560, 78, 575, 93, 0);
	api_putstrwin(win, 580, 78, 0, 14, "allocated(black)");
	api_boxfilwin(win, 560, 102, 575, 117, 7);
	api_putstrwin(win, 580, 102, 0, 11, "free(white)");

	sprintf(s, "unit: %dKB/pixel", UNIT_BYTES / 1024);
	api_putstrwin(win, 560, 130, 0, strlen(s), s);
	sprintf(s, "total: 128MB");
	api_putstrwin(win, 560, 146, 0, strlen(s), s);
	api_putstrwin(win, 560, 226, 0, 22, "press key to close(1s)");

	timer = api_alloctimer();
	api_inittimer(timer, 128);

	for (;;) {
		for (i = 0; i < PAGE_COUNT; i++) {
			state[i] = 1;
		}

		free_bytes = 0;
		entry_count = api_getmemmap(entries, MAX_FREE_ENTRIES, &free_bytes);
		for (i = 0; i < entry_count; i++) {
			start_page = entries[i].addr / UNIT_BYTES;
			end_page = (entries[i].addr + entries[i].size + UNIT_BYTES - 1) / UNIT_BYTES;
			if (start_page >= PAGE_COUNT) {
				continue;
			}
			if (end_page > PAGE_COUNT) {
				end_page = PAGE_COUNT;
			}
			for (j = (int) start_page; j < (int) end_page; j++) {
				state[j] = 0;
			}
		}

		free_pages = 0;
		for (i = 0; i < PAGE_COUNT; i++) {
			if (state[i] == 0) {
				free_pages++;
			}
		}
		alloc_pages = PAGE_COUNT - free_pages;

		draw_map(winbuf, state);
		api_refreshwin(win, MAP_X - 2, MAP_Y - 2, MAP_X + MAP_W + 2, MAP_Y + MAP_H + 2);

		api_boxfilwin(win, 560, 162, 750, 210, 8);
		sprintf(s, "alloc: %dMB", (alloc_pages * 4) / 1024);
		api_putstrwin(win, 560, 162, 0, strlen(s), s);
		sprintf(s, "free : %dMB", (free_pages * 4) / 1024);
		api_putstrwin(win, 560, 178, 0, strlen(s), s);
		sprintf(s, "free blocks: %d", entry_count);
		api_putstrwin(win, 560, 194, 0, strlen(s), s);

		api_settimer(timer, 100);
		key = api_getkey(1);
		if (key != 128) {
			break;
		}
	}

	api_freetimer(timer);
	api_end();
	return 0;
}
