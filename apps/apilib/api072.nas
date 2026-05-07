[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api072.nas"]

        GLOBAL  _api_memfs_create

[SECTION .text]

_api_memfs_create:    ; int api_memfs_create(char *path);
        PUSH    EBX
        MOV     EDX,72
        MOV     EBX,[ESP+8]
        INT     0x40
        POP     EBX
        RET
