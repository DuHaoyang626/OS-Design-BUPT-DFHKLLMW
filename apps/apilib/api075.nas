[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api075.nas"]

        GLOBAL  _api_memfs_copy

[SECTION .text]

_api_memfs_copy:      ; int api_memfs_copy(char *src, char *dst);
        PUSH    EBX
        MOV     EDX,75
        MOV     EBX,[ESP+8]     ; src
        MOV     ECX,[ESP+12]    ; dst
        INT     0x40
        POP     EBX
        RET
