[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api070.nas"]

        GLOBAL  _api_memfs_format

[SECTION .text]

_api_memfs_format:    ; int api_memfs_format(int disk_kb);
        MOV     EDX,70
        MOV     EAX,[ESP+4]
        INT     0x40
        RET
