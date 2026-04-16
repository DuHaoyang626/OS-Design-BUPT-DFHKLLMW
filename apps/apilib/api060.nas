[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api060.nas"]

		GLOBAL	_api_syncdemo_start

[SECTION .text]

_api_syncdemo_start:	; void api_syncdemo_start(void);
		MOV		EDX,60
		INT		0x40
		RET