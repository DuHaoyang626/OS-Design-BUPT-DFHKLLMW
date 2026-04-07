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

struct TASKCTL *taskctl;
struct TIMER *task_timer;
// 按层级返回时间片的量子表，高层任务短时间片，低层任务长时间片
#define TASK_AGING_TICKS 100

static int task_quantum_for_level(int level)
{
	static const int quantum_table[MAX_TASKLEVELS] = {
			2, 2, 4, 4, 8, 8, 16, 16, 32, 1};

	if (level < 0)
	{
		level = 0;
	}
	else if (level >= MAX_TASKLEVELS)
	{
		level = MAX_TASKLEVELS - 1;
	}
	return quantum_table[level];
}

static int task_aging_limit(int level)
{
	return TASK_AGING_TICKS * (level + 1);
}

int task_aging_limit_for_level(int level)
{
	if (level < 0)
	{
		level = 0;
	}
	else if (level >= MAX_TASKLEVELS)
	{
		level = MAX_TASKLEVELS - 1;
	}
	return task_aging_limit(level);
}

// 任务老化函数�?
// 检查所有就绪队列中的任务，如果它们等待的时间超过了对应层级的老化限制，就将它们提升到更高的层级�?
static void task_apply_aging(struct TASK *current_task)
{
	unsigned int now = timerctl.count;
	int level, i;

	for (level = 1; level < MAX_TASKLEVELS; level++)
	{
		struct TASKLEVEL *tl = &taskctl->level[level];
		for (i = 0; i < tl->running;)
		{
			struct TASK *task = tl->tasks[i];
			if (task != current_task && now - task->enqueue_tick >= (unsigned int)task_aging_limit(level))
			{
				task_run(task, level - 1, 0);
				continue;
			}
			i++;
		}
	}
}

struct TASK *task_now(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->enqueue_tick = timerctl.count; // 记录任务进入就绪队列的时间，每次重新入队重新开始等待计时�?
	task->flags = 2;										 /* ���쒆 */
	return;
}

void task_remove(struct TASK *task)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	/* task���ǂ��ɂ��邩��T�� */
	for (i = 0; i < tl->running; i++)
	{
		if (tl->tasks[i] == task)
		{
			/* �����ɂ��� */
			break;
		}
	}

	tl->running--;
	if (i < tl->now)
	{
		tl->now--; /* �����̂ŁA��������킹�Ă���?*/
	}
	if (tl->now >= tl->running)
	{
		/* now���������Ȓl�ɂȂ��Ă�����A�C������ */
		tl->now = 0;
	}
	task->flags = 1; /* �X���[�v�� */

	/* ���炵 */
	for (; i < tl->running; i++)
	{
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

void task_switchsub(void)
{
	int i;
	/* ��ԏ�̃��x����T�� */
	for (i = 0; i < MAX_TASKLEVELS; i++)
	{
		if (taskctl->level[i].running > 0)
		{
			break; /* �������� */
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}

void task_idle(void)
{
	for (;;)
	{
		io_hlt();
	}
}

struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;

	taskctl = (struct TASKCTL *)memman_alloc_4k(memman, sizeof(struct TASKCTL));
	for (i = 0; i < MAX_TASKS; i++)
	{
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int)taskctl->tasks0[i].ldt, AR_LDT);
	}
	for (i = 0; i < MAX_TASKLEVELS; i++)
	{
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}

	task = task_alloc();
	task->flags = 2;														/* ���쒆�}�[�N */
	task->priority = task_quantum_for_level(0); // 把初始任务和 idle 任务的时间片改成由层级自动决定，不再手工固定�?
	task->level = 0;														/* �ō����x�� */
	task_add(task);
	task_switchsub(); /* ���x���ݒ� */
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);

	idle = task_alloc();
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int)&task_idle;
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	task_run(idle, MAX_TASKLEVELS - 1, 0); // idle 任务放在最低层，时间片最长�?

	return task;
}

struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++)
	{
		if (taskctl->tasks0[i].flags == 0)
		{
			task = &taskctl->tasks0[i];
			task->flags = 1;							 /* �g�p���}�[�N */
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0;						 /* �Ƃ肠����0�ɂ��Ă������Ƃɂ��� */
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.iomap = 0x40000000;
			task->tss.ss0 = 0;
			return task;
		}
	}
	return 0; /* �����S���g�p�� */
}

void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0)
	{
		level = task->level; /* ���x����ύX���Ȃ*/
	}
	if (priority > 0)
	{
		task->priority = priority;
	}
	else
	{
		task->priority = task_quantum_for_level(level);
	}

	if (task->flags == 2 && task->level != level)
	{										 /* ���쒆�̃��x���̕ύX */
		task_remove(task); /* ��������s�����flags��1�ɂȂ�̂ŉ���if�����s�����?*/
	}
	if (task->flags != 2)
	{
		/* �X���[�v����N�������ꍇ */
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; /* ����^�X�N�X�C�b�`�̂Ƃ��Ƀ��x���������� */
	return;
}

void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2)
	{
		/* ���쒆�������� */
		now_task = task_now();
		task_remove(task); /* ��������s�����flags��1�ɂȂ� */
		if (task == now_task)
		{
			/* �������g�̃X���[�v�������̂ŁA�^�X�N�X�C�b�`���K�v */
			task_switchsub();
			now_task = task_now(); /* �ݒ��ł́A�u���݂̃^�X�N�v�������Ă��炤 */
			farjmp(0, now_task->sel);
		}
	}
	return;
}

// 把启动时�?task_run 调用改成不再硬编码时间片，让调度器自己根据层级决定�?
void task_switch(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];

	if (g_sched_enable_aging)
	{
		task_apply_aging(now_task);
	}

	if (now_task->level < MAX_TASKLEVELS - 1)
	{
		/* ������һ������ʱ��Ƭ������ */
		task_run(now_task, now_task->level + 1, 0);
	}
	else
	{
		/* ��ͼ�����ת */
		tl->now++;
		if (tl->now == tl->running)
		{
			tl->now = 0;
		}
	}

	if (taskctl->lv_change != 0)
	{
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}

	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task)
	{
		farjmp(0, new_task->sel);
	}
	return;
}
