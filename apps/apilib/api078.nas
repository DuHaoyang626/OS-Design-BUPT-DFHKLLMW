[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api078.nas"]

        GLOBAL  _api_getdsbase

[SECTION .text]

_api_getdsbase:     ; int api_getdsbase(void);
        MOV     EDX,78
        INT     0x40
        RET