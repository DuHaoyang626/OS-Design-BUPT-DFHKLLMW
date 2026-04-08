[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api077.nas"]

        GLOBAL  _api_memfs_list

[SECTION .text]

_api_memfs_list:      ; int api_memfs_list(char *path, char *outbuf, int outbuf_len);
        PUSH    EBX
        MOV     EDX,77
        MOV     EBX,[ESP+8]     ; path
        MOV     ECX,[ESP+12]    ; outbuf
        MOV     EAX,[ESP+16]    ; outbuf_len
        INT     0x40
        POP     EBX
        RET
