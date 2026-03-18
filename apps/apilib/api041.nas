[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api041.nas"]

		GLOBAL	_api_getmemalgo

[SECTION .text]

_api_getmemalgo:		; int api_getmemalgo(void);
		PUSH	EBX
		MOV		EDX,37
		INT		0x40
		POP		EBX
		RET
