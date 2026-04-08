[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api076.nas"]

        GLOBAL  _api_memfs_delete

[SECTION .text]

_api_memfs_delete:    ; int api_memfs_delete(char *path);
        PUSH    EBX
        MOV     EDX,76
        MOV     EBX,[ESP+8]
        INT     0x40
        POP     EBX
        RET
