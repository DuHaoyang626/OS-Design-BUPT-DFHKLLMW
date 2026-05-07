[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api066.nas"]

		GLOBAL	_api_pc_init

[SECTION .text]

_api_pc_init:	; void api_pc_init(void);
		MOV		EDX,66
		INT		0x40
		RET