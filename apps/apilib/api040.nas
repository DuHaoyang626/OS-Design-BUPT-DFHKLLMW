[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api040.nas"]

		GLOBAL	_api_getmemmap

[SECTION .text]

_api_getmemmap:		; int api_getmemmap(struct MEMMAP_ENTRY *entries, int max_entries, int *free_bytes);
		PUSH	EBX
		MOV		EDX,36
		MOV		EAX,[ESP+8]			; entries
		MOV		ECX,[ESP+12]		; max_entries
		MOV		EBX,[ESP+16]		; free_bytes
		INT		0x40
		POP		EBX
		RET
