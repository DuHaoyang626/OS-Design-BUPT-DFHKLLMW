[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api063.nas"]

		GLOBAL	_api_sem_wait

[SECTION .text]

_api_sem_wait:		; void api_sem_wait(void);
		MOV		EDX,63
		INT		0x40
		RET