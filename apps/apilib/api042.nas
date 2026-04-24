[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api042.nas"]

		GLOBAL	_api_getmmumode

[SECTION .text]

_api_getmmumode:		; int api_getmmumode(void);
		PUSH	EBX
		MOV		EDX,38
		INT		0x40
		POP		EBX
		RET