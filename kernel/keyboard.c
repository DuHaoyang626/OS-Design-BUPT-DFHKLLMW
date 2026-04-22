///////////////////////////////////////////
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

struct FIFO32 *keyfifo;
int keydata0;

/* 去抖动：记录上一次按键扫描码和时间，同一键在 DEBOUNCE_TICKS 内只接受一次 */
#define DEBOUNCE_TICKS 3
static int last_keydata = -1;
static unsigned int last_keytime = 0;

void inthandler21(int *esp)
{
	int data;
	io_out8(PIC0_OCW2, 0x61);
	data = io_in8(PORT_KEYDAT);
	/* 只对 make code（< 0x80）做去抖动，break code 正常放行 */
	if (data < 0x80) {
		if (data == last_keydata &&
			timerctl.count - last_keytime < DEBOUNCE_TICKS) {
			return; /* 丢弃重复 */
		}
		last_keydata = data;
		last_keytime = timerctl.count;
	} else {
		last_keydata = -1; /* 松键后重置，允许下次按同一键 */
	}
	fifo32_put(keyfifo, data + keydata0);
	return;
}

#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready(void)
{
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(struct FIFO32 *fifo, int data0)
{
	keyfifo = fifo;
	keydata0 = data0;
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}
