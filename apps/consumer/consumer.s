	.file	"consumer.c"
	.data
LC0:
	.ascii "Consumed: %d\12\0"
	.text
	.balign 2
.globl __main
	.def	__main;	.scl	2;	.type	32;	.endef
__main:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%ebx
	movl	$14, %esi
	subl	$32, %esp
L6:
	leal	-40(%ebp), %ebx
	call	_api_pc_consume
	pushl	%eax
	pushl	$LC0
	pushl	%ebx
	call	_sprintf
	pushl	%ebx
	call	_api_putstr0
	addl	$16, %esp
	decl	%esi
	jns	L6
	call	_api_end
	leal	-8(%ebp), %esp
	popl	%ebx
	popl	%esi
	popl	%ebp
	ret
	.def	_api_end;	.scl	2;	.type	32;	.endef
	.def	_api_putstr0;	.scl	2;	.type	32;	.endef
	.def	_sprintf;	.scl	2;	.type	32;	.endef
	.def	_api_pc_consume;	.scl	2;	.type	32;	.endef
