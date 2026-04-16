#######################
# HELO OS Makefile
# B Y : S T O N
#######################

TOOLPATH = ./z_tools/
INCPATH  = ./z_tools/HeloInclude/

MAKE     = $(TOOLPATH)make.exe -r
EDIMG    = $(TOOLPATH)edimg.exe
COPY     = copy
DEL      = del

# Memory allocator strategy passed to kernel build.
# Options: LEGACY, FIRST_FIT, NEXT_FIT, BEST_FIT, WORST_FIT
MEM_ALLOC_ALGO ?= LEGACY

APP_DIRS = 2dball about calc counter cpuid csvv cvtg date gview hview invader mtorz music note pc tview type video
APP_HELS = \
	apps/2dball/2dball.hel \
	apps/about/about.hel \
	apps/calc/calc.hel \
	apps/counter/counter.hel \
	apps/cpuid/cpuid.hel \
	apps/csvv/csvv.hel \
	apps/cvtg/cvtg.hel \
	apps/date/date.hel \
	apps/gview/gview.hel \
	apps/hview/hview.hel \
	apps/invader/invader.hel \
	apps/memmap/memmap.hel \
	apps/mtorz/mtorz.hel \
	apps/music/music.hel \
	apps/note/note.hel \
	apps/pc/pc.hel \
	apps/tview/tview.hel \
	apps/type/type.hel \
	apps/video/video.hel

.PHONY: default full apps libs kernel image run run_full clean

default : Helo_OS.img

libs :
	$(MAKE) -C ./apps/apilib
	$(MAKE) -C ./apps/stdlib

apps : libs
	$(MAKE) -C ./apps/2dball
	$(MAKE) -C ./apps/about
	$(MAKE) -C ./apps/calc
	$(MAKE) -C ./apps/counter
	$(MAKE) -C ./apps/cpuid
	$(MAKE) -C ./apps/csvv
	$(MAKE) -C ./apps/cvtg
	$(MAKE) -C ./apps/date
	$(MAKE) -C ./apps/gview
	$(MAKE) -C ./apps/hview
	$(MAKE) -C ./apps/invader
	$(MAKE) -C ./apps/memmap
	$(MAKE) -C ./apps/mtorz
	$(MAKE) -C ./apps/music
	$(MAKE) -C ./apps/note
	$(MAKE) -C ./apps/pc
	$(MAKE) -C ./apps/tview
	$(MAKE) -C ./apps/type
	$(MAKE) -C ./apps/video

kernel :
	$(MAKE) -C ./kernel MEM_ALLOC_ALGO=$(MEM_ALLOC_ALGO)

image : Helo_OS.img

full : apps kernel Helo_OS.img

Helo_OS.img : kernel/ipl20.bin kernel/Helo_OS.sys $(APP_HELS) Makefile
	$(EDIMG)   imgin:./z_tools/fdimg0at.tek \
		wbinimg src:kernel/ipl20.bin len:512 from:0 to:0 \
		copy from:kernel/Helo_OS.sys to:@: \
		copy from:kernel/ipl20.nas to:@: \
		copy from:apps/type/type.hel to:@: \
		copy from:apps/date/date.hel to:@: \
		copy from:apps/tview/tview.hel to:@: \
		copy from:apps/gview/gview.hel to:@: \
		copy from:apps/about/about.hel to:@: \
		copy from:apps/music/music.hel to:@: \
		copy from:apps/invader/invader.hel to:@: \
		copy from:apps/calc/calc.hel to:@: \
		copy from:apps/pc/pc.hel to:@: \
		copy from:apps/pc/logo.pzk to:@: \
		copy from:apps/2dball/2dball.hel to:@: \
		copy from:apps/hview/hview.hel to:@: \
		copy from:apps/note/note.hel to:@: \
		copy from:apps/csvv/csvv.hel to:@: \
		copy from:apps/cpuid/cpuid.hel to:@: \
		copy from:apps/cvtg/cvtg.hel to:@: \
	copy from:apps/counter/counter.hel to:@: \
	copy from:apps/syncdemo/syncdemo.hel to:@: \
	copy from:apps/video/video.hel to:@: \
		copy from:apps/memmap/memmap.hel to:@: \
		copy from:apps/mtorz/mtorz.hel to:@: \
		copy from:data/daigo.mld to:@: \
		copy from:data/daiku.mld to:@: \
		copy from:data/star.mld to:@: \
		copy from:data/moon.txt to:@: \
		copy from:data/dog.jpg to:@: \
		copy from:data/helo.vdo to:@: \
		copy from:data/help.txt to:@: \
		copy from:data/chinese/HZK16.fnt to:@: \
		copy from:data/heloos.jpg to:@: \
		copy from:data/LOGO.bmp to:@: \
		copy from:data/1.csv to:@: \
		imgout:Helo_OS.img

run : Helo_OS.img
	$(COPY) Helo_OS.img .\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ./z_tools/qemu

clean :
	-$(MAKE) -C ./apps/2dball src_only
	-$(MAKE) -C ./apps/about src_only
	-$(MAKE) -C ./apps/calc src_only
	-$(MAKE) -C ./apps/counter src_only
	-$(MAKE) -C ./apps/cpuid src_only
	-$(MAKE) -C ./apps/csvv src_only
	-$(MAKE) -C ./apps/cvtg src_only
	-$(MAKE) -C ./apps/date src_only
	-$(MAKE) -C ./apps/gview src_only
	-$(MAKE) -C ./apps/hview src_only
	-$(MAKE) -C ./apps/invader src_only
	-$(MAKE) -C ./apps/memmap src_only
	-$(MAKE) -C ./apps/mtorz src_only
	-$(MAKE) -C ./apps/music src_only
	-$(MAKE) -C ./apps/note src_only
	-$(MAKE) -C ./apps/pc src_only
	-$(MAKE) -C ./apps/tview src_only
	-$(MAKE) -C ./apps/type src_only
	-$(MAKE) -C ./apps/video src_only
	-$(MAKE) -C ./apps/apilib src_only
	-$(MAKE) -C ./apps/stdlib src_only
	-$(MAKE) -C ./kernel src_only
	-$(DEL) Helo_OS.img

run_full : clean full run