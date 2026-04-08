[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api074.nas"]

        GLOBAL  _api_memfs_read

[SECTION .text]

_api_memfs_read:      ; int api_memfs_read(char *path, int offset, char *buf, int len);
        PUSH    EBX
        PUSH    ESI
        MOV     EDX,74
        MOV     EBX,[ESP+12]    ; path
        MOV     EAX,[ESP+16]    ; offset
        MOV     ECX,[ESP+20]    ; buf
        MOV     ESI,[ESP+24]    ; len
        INT     0x40
        POP     ESI
        POP     EBX
        RET
