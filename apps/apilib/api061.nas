[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api061.nas"]

		GLOBAL	_api_get_shared

[SECTION .text]

_api_get_shared:	; int api_get_shared(void);
		MOV		EDX,61
		INT		0x40
		RET