#######################
# HELO OS Makefile�ļ�
# B Y : S T O N
#######################


OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj \
		int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj \
		mtask.obj window.obj console.obj file.obj tek.obj jpeg.obj cmos.obj

TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/HeloInclude/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
MEM_ALLOC_ALGO ?= LEGACY

ifeq ($(MEM_ALLOC_ALGO),FIRST_FIT)
ALLOC_DEF = -DMEM_ALLOC_ALGO=1
else
	ifeq ($(MEM_ALLOC_ALGO),NEXT_FIT)
	ALLOC_DEF = -DMEM_ALLOC_ALGO=2
	else
		ifeq ($(MEM_ALLOC_ALGO),BEST_FIT)
		ALLOC_DEF = -DMEM_ALLOC_ALGO=3
		else
			ifeq ($(MEM_ALLOC_ALGO),WORST_FIT)
			ALLOC_DEF = -DMEM_ALLOC_ALGO=4
			else
			ALLOC_DEF = -DMEM_ALLOC_ALGO=0
			endif
		endif
	endif
endif

CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet $(ALLOC_DEF)
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
BIM2HEL  = $(TOOLPATH)bim2hel.exe
RULEFILE = ../kernel_rule_file.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del


default :
	$(MAKE) ipl20.bin
	$(MAKE) Helo_OS.sys


ipl20.bin : ipl20.nas Makefile
	$(NASK) ipl20.nas ipl20.bin ipl20.lst

asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

bootpack.hel : bootpack.bim Makefile
	$(BIM2HEL) bootpack.bim bootpack.hel 0

Helo_OS.sys : asmhead.bin bootpack.hel Makefile
	copy /B asmhead.bin+bootpack.hel Helo_OS.sys


%.gas : %.c bootpack.h Makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst


clean :
	-$(DEL) asmhead.bin
	-$(DEL) hankaku.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hel

src_only :
	$(MAKE) clean
	-$(DEL) ipl20.bin
	-$(DEL) Helo_OS.sys