section .data
red_color:
	db	1Bh, '[31;1m', 0
.len	equ	$ - red_color
default_color:
	db	1Bh, '[37;0m', 0
.len	equ	$ - default_color

global	asm_print

section .text
asm_print:
	mov	eax, [esp+12]
	cmp	eax, 0
	je	print
	call	to_red
	jmp	print
	call 	to_default
	
print:
	mov	ecx, [esp+4]
	mov	edx, [esp+8]
	mov	ebx, 1
	mov	eax, 4
	int 80h
	call 	to_default
	ret

to_red:
	mov	eax, 4
	mov	ebx, 1
	mov	ecx, red_color
	mov	edx, red_color.len
	int 80h
	ret

to_default:
	mov	eax, 4
	mov 	ebx, 1
	mov	ecx, default_color
	mov	edx, default_color.len
	int 80h
	ret
