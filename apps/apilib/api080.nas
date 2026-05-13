[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api080.nas"]

        GLOBAL  _api_virt2phys

[SECTION .text]

_api_virt2phys:     ; unsigned int api_virt2phys(void *ptr);
        MOV     EDX,80
        MOV     EBX,[ESP+4]
        INT     0x40
        RET