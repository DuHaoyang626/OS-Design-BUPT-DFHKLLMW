[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api039.nas"]

		GLOBAL	_asm_rdtsc

[SECTION .text]

_asm_rdtsc:	; void asm_rdtsc(int *high, int *low);
		PUSHAD
		MOV		EBP,[ESP+36]
		DB		0x0F, 0x31			; RDTSC
		MOV		[EBP],EDX			;
		MOV		[EBP+4],EAX			;
		POPAD
		RET
