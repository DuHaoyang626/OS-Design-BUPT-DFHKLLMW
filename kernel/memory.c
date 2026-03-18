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

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

#ifndef MEM_ALLOC_ALGO
#define MEM_ALLOC_ALGO 0
#endif

#define MEM_ALLOC_ALGO_LEGACY	0
#define MEM_ALLOC_ALGO_FIRST_FIT	1
#define MEM_ALLOC_ALGO_NEXT_FIT	2
#define MEM_ALLOC_ALGO_BEST_FIT	3
#define MEM_ALLOC_ALGO_WORST_FIT	4

static int memman_rr_next = 0;

static void memman_remove_free(struct MEMMAN *man, int i)
{
	for (; i < man->frees - 1; i++) {
		man->free[i] = man->free[i + 1];
	}
	man->frees--;
}

static unsigned int memman_alloc_legacy(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < (unsigned int)man->frees; i++) {
		if (man->free[i].size >= size) {
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				memman_remove_free(man, i);
			}
			return a;
		}
	}
	return 0;
}

static unsigned int memman_alloc_first_fit(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < (unsigned int)man->frees; i++) {
		if (man->free[i].size >= size) {
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				memman_remove_free(man, i);
			}
			return a;
		}
	}
	return 0;
}

static unsigned int memman_alloc_next_fit(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a, start, idx;
	if (man->frees <= 0) {
		return 0;
	}
	if (memman_rr_next >= man->frees) {
		memman_rr_next = 0;
	}
	start = (unsigned int)memman_rr_next;
	for (i = 0; i < (unsigned int)man->frees; i++) {
		idx = (start + i) % (unsigned int)man->frees;
		if (man->free[idx].size >= size) {
			a = man->free[idx].addr;
			man->free[idx].addr += size;
			man->free[idx].size -= size;
			if (man->free[idx].size == 0) {
				memman_remove_free(man, idx);
			}
			if (man->frees <= 0) {
				memman_rr_next = 0;
			} else if ((int)idx >= man->frees) {
				memman_rr_next = 0;
			} else {
				memman_rr_next = (int)idx;
			}
			return a;
		}
	}
	return 0;
}

static unsigned int memman_alloc_best_fit(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a, idx = 0, best_size = 0xffffffff;
	char found = 0;
	for (i = 0; i < (unsigned int)man->frees; i++) {
		if (man->free[i].size >= size && man->free[i].size < best_size) {
			best_size = man->free[i].size;
			idx = i;
			found = 1;
		}
	}
	if (found == 0) {
		return 0;
	}
	a = man->free[idx].addr;
	man->free[idx].addr += size;
	man->free[idx].size -= size;
	if (man->free[idx].size == 0) {
		memman_remove_free(man, idx);
	}
	return a;
}

static unsigned int memman_alloc_worst_fit(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a, idx = 0, worst_size = 0;
	char found = 0;
	for (i = 0; i < (unsigned int)man->frees; i++) {
		if (man->free[i].size >= size && man->free[i].size > worst_size) {
			worst_size = man->free[i].size;
			idx = i;
			found = 1;
		}
	}
	if (found == 0) {
		return 0;
	}
	a = man->free[idx].addr;
	man->free[idx].addr += size;
	man->free[idx].size -= size;
	if (man->free[idx].size == 0) {
		memman_remove_free(man, idx);
	}
	return a;
}

int memman_get_algo_id(void)
{
	return MEM_ALLOC_ALGO;
}

char *memman_get_algo_name(void)
{
	if (MEM_ALLOC_ALGO == MEM_ALLOC_ALGO_FIRST_FIT) {
		return "FIRST_FIT";
	}
	if (MEM_ALLOC_ALGO == MEM_ALLOC_ALGO_NEXT_FIT) {
		return "NEXT_FIT";
	}
	if (MEM_ALLOC_ALGO == MEM_ALLOC_ALGO_BEST_FIT) {
		return "BEST_FIT";
	}
	if (MEM_ALLOC_ALGO == MEM_ALLOC_ALGO_WORST_FIT) {
		return "WORST_FIT";
	}
	return "LEGACY";
}

unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 386���A486�ȍ~�Ȃ̂��̊m�F */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 386�ł�AC=1�ɂ��Ă�������0�ɖ߂��Ă��܂� */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* �L���b�V���֎~ */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* �L���b�V������ */
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* �������̌� */
	man->maxfrees = 0;		/* �󋵊ώ@�p�Ffrees�̍ő�l */
	man->lostsize = 0;		/* ����Ɏ��s�������v�T�C�Y */
	man->losts = 0;			/* ����Ɏ��s������ */
	memman_rr_next = 0;
	return;
}

unsigned int memman_total(struct MEMMAN *man)
/* �����T�C�Y�̍��v��� */
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
/* �m�� */
{
#if MEM_ALLOC_ALGO == MEM_ALLOC_ALGO_FIRST_FIT
	return memman_alloc_first_fit(man, size);
#elif MEM_ALLOC_ALGO == MEM_ALLOC_ALGO_NEXT_FIT
	return memman_alloc_next_fit(man, size);
#elif MEM_ALLOC_ALGO == MEM_ALLOC_ALGO_BEST_FIT
	return memman_alloc_best_fit(man, size);
#elif MEM_ALLOC_ALGO == MEM_ALLOC_ALGO_WORST_FIT
	return memman_alloc_worst_fit(man, size);
#else
	return memman_alloc_legacy(man, size);
#endif
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* ��� */
{
	int i, j;
	/* �܂Ƃ߂₷�����l����ƁAfree[]��addr���ɕ���ł���ق������� */
	/* ������܂��A�ǂ��ɓ����ׂ��������߂� */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* �O������ */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* �O�̂����̈�ɂ܂Ƃ߂��� */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* �������� */
				if (addr + size == man->free[i].addr) {
					/* �Ȃ�ƌ��Ƃ��܂Ƃ߂��� */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]�̍폜 */
					/* free[i]���Ȃ��Ȃ����̂őO�ւ߂� */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* �\���̂̑�� */
					}
				}
			}
			return 0; /* �����I�� */
		}
	}
	/* �O�Ƃ͂܂Ƃ߂��Ȃ����� */
	if (i < man->frees) {
		/* ��낪���� */
		if (addr + size == man->free[i].addr) {
			/* ���Ƃ͂܂Ƃ߂��� */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* �����I�� */
		}
	}
	/* �O�ɂ����ɂ��܂Ƃ߂��Ȃ� */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]�������A���ւ��炵�āA�����܂���� */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* �ő�l���X�V */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* �����I�� */
	}
	/* ���ɂ��点�Ȃ����� */
	man->losts++;
	man->lostsize += size;
	return -1; /* ���s�I�� */
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}
