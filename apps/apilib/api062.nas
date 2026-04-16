[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api062.nas"]

		GLOBAL	_api_set_shared

[SECTION .text]

_api_set_shared:	; void api_set_shared(int val);
		MOV		EDX,62
		MOV		EAX,[ESP+4]		; val
		INT		0x40
		RET