[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api067.nas"]

		GLOBAL	_api_pc_produce

[SECTION .text]

_api_pc_produce:	; void api_pc_produce(int val);
		MOV		EDX,67
		MOV		EAX,[ESP+4]		; val
		INT		0x40
		RET