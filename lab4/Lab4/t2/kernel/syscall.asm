
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

INT_VECTOR_SYS_CALL equ 0x90
_NR_get_ticks       equ 0
_NR_Print	    equ 1
_NR_Sleep           equ 2
_NR_P               equ 3
_NR_V               equ 4

; 导出符号
global	get_ticks
global	myprint
global  mysleep
global  P
global  V

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

myprint:
	mov	eax,_NR_Print
	push ebx
	push ecx
	mov	ebx,[esp+12]	;字符串
	mov	ecx,[esp+16]	;颜色
	int	INT_VECTOR_SYS_CALL
	pop	ecx
	pop	ebx
	ret

mysleep:
	mov	eax,_NR_Sleep
	push ebx
	mov	ebx,[esp+8]
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret

P:
	push ebx
	mov eax, _NR_P
	mov ebx, [esp+8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret

V:
	push ebx
	mov eax, _NR_V
	mov ebx, [esp+8]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret

; ====================================================================================
;                          void write(char* buf, int len);
; ====================================================================================
; write:
;         mov     eax, _NR_write
;         mov     ebx, [esp + 4]
;         mov     ecx, [esp + 8]
;         int     INT_VECTOR_SYS_CALL
;         ret
