[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api064.nas"]

		GLOBAL	_api_sem_post

[SECTION .text]

_api_sem_post:		; void api_sem_post(void);
		MOV		EDX,64
		INT		0x40
		RET