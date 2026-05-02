[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api079.nas"]

        GLOBAL  _api_memfs_release

[SECTION .text]

_api_memfs_release:   ; int api_memfs_release(void);
        MOV     EDX,79
        MOV     EAX,[ESP+4]
        INT     0x40
        RET
