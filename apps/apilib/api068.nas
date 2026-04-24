[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api068.nas"]

		GLOBAL	_api_pc_consume

[SECTION .text]

_api_pc_consume:	; int api_pc_consume(void);
		MOV		EDX,68
		INT		0x40
		RET