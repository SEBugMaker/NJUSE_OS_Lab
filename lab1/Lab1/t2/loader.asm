        org     10000h

            mov     ax,     cs
            mov     ds,     ax
            mov     es,     ax
            mov     ax,     0x00
            mov     ss,     ax
            mov     sp,     0x7c00

        ;=======    display on screen : Start Loader......

            mov     ax,     1301h
            mov     bx,     004fh
            mov     dx,     0911h          ;row 2
            mov     cx,     13
            push    ax
            mov     ax,     ds
            mov     es,     ax
            pop     ax
            mov     bp,     StartLoaderMessage
            int     10h

            jmp     $

        ;=======    display messages

        StartLoaderMessage:     db     "Hello Loader!"
        
times 	510-($-$$)	db	0	; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw 	0xaa55				; 结束标志
