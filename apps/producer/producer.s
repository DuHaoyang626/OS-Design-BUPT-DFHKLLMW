	.file	"producer.c"
	.text
	.balign 2
.globl _HariMain
	.def	_HariMain;	.scl	2;	.type	32;	.endef
_HariMain:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	xorl	%ebx, %ebx
	call	_api_pc_init
L6:
	pushl	%ebx
	incl	%ebx
	call	_api_pc_produce
	cmpl	$14, %ebx
	popl	%eax
	jle	L6
	movl	-4(%ebp), %ebx
	leave
	jmp	_api_end
	.def	_api_end;	.scl	2;	.type	32;	.endef
	.def	_api_pc_produce;	.scl	2;	.type	32;	.endef
	.def	_api_pc_init;	.scl	2;	.type	32;	.endef
