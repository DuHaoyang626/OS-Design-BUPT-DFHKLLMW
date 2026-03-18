[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api038.nas"]

		GLOBAL	_asm_cpuid

[SECTION .text]

_asm_cpuid:	; void asm_cpuid(int id_eax, int id_ecx, int *eax, int *ebx, int *ecx, int *edx);
		PUSHAD
		MOV		EAX,[ESP+36]		; id_eax
		MOV		ECX,[ESP+40]		; id_ecx
		MOV		EBP,[ESP+44]
		DB		0x0F, 0xA2			; CPUID
		MOV		[EBP],EAX			;
		MOV		[EBP+4],EBX			;
		MOV		[EBP+8],ECX			;
		MOV		[EBP+12],EDX		;
		POPAD
		RET