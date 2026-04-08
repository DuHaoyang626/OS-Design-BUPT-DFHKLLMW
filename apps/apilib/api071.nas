[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api071.nas"]

        GLOBAL  _api_memfs_mkdir

[SECTION .text]

_api_memfs_mkdir:     ; int api_memfs_mkdir(char *path);
        PUSH    EBX
        MOV     EDX,71
        MOV     EBX,[ESP+8]
        INT     0x40
        POP     EBX
        RET
