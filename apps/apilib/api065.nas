[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api065.nas"]

		GLOBAL	_api_sync_reset

[SECTION .text]

_api_sync_reset:	; void api_sync_reset(void);
		MOV		EDX,65
		INT		0x40
		RET