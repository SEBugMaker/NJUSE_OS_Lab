%include 'function.asm'

SECTION .data
space db ' ',0h
zero: db 48,0h
;====================
src: times 105 db 0h
dest: times 105 db 0h 
; src / dest
res_R: times 105 db 0h;商
res_Q: times 105 db 0h;余数
;====================
temp: times 105 db 0h
count:db 0h

error_message1: db '请输入数字',0ah,0h;报错信息
error_message2: db '除数不能为零',0ah,0h;报错信息

SECTION .bss
input:     resb    255   

SECTION .text
global _start
_start:
        mov     edx, 255        
        mov     ecx, input      ; 读入字符串，写入input
        mov     ebx, 0          
        mov     eax, 3          
        int     80h
        ;input and check
        mov edx,0
        call split
        call check

        ;special condition -> src smaller than dest -> R=0, Q=
        mov eax, dest
        mov ebx, src
        call compare_num
        cmp dl, 1
        jnz Normal_Condition  ;-> normal condition

        ;%%%%%%%%%%%%%
        mov eax,zero
        mov ebx,src
        call print_res
        jmp end1
        ;%%%%%%%%%%%%%

        Normal_Condition:
        mov eax,src
        mov ebx,dest
        mov dl,48
        mov [res_R],dl
        call div_func
        jmp end1

        end1:
        mov ebx,0
        mov eax,1
        int 80h

split:
        
       
        mov eax, input
        mov ebx, src
        mov ecx,0
        nextchar_src:      
        cmp byte [eax], ' ' ; 和空格比较
        jz nextchar_pre_dest
        mov cl ,byte [eax]
        mov byte [ebx], cl
        inc ebx
        inc eax
        jmp nextchar_src

        nextchar_pre_dest:
        mov ecx, 0
        mov ebx, dest
        inc eax
        nextchar_dest:
        cmp byte [eax], 10
        jz  finished_input
        mov cl ,byte [eax]
        mov byte [ebx], cl
        inc eax
        inc ebx
        jmp nextchar_dest

        finished_input:
        ret

error:
        ;error resolve
        error1:
        ;output error that exists no-number char
        mov eax,error_message1
        call sprint
        jmp end1

        error2:
        ;output error that the dest is zero
        mov eax,error_message2
        call sprint
        jmp end1

        ret

check:
        ;check src
        ;===================================			

        mov eax, src
        mov bl, [eax]
        mov ecx,eax
        mov eax,ecx

        next_char_src:
        mov bl, [eax]
        cmp byte [eax],0
        jz finished_judge_src
        ;check whether the character is number
        ;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        cmp bl,30h
        jl err1_src
        cmp bl,39h
        jg err1_src
        ;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        inc eax
        jmp next_char_src

        err1_src:
        mov dh,1
        jmp finished_judge_src
        
        finished_judge_src:
        
        cmp dh, 1
        jz error1
        cmp dh, 2
        jz error2

        ;check dest
        ;===================================
        
        mov eax, dest
        mov cl, [eax]
        ;check if zero
        ;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        cmp cl, 48
        jz ifZeroSrc
        ;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        mov ecx,eax
        mov eax,ecx
        
        nextchar_Dest:
        mov cl, [eax]
        cmp byte [eax],0
        jz finished_judge_Dest
        ; 字符串应该为数字字符
        cmp cl, 30h
        jl err1Dest
        cmp cl, 39h
        jg err1Dest
        ;==================
        inc eax
        jmp nextchar_Dest
        
        err1Dest:
        mov dh,1
        jmp finished_judge_Dest

        ifZeroSrc:
        mov ecx,eax
        inc ecx
        mov bl,[ecx]
        cmp bl,0
        jz err2Dest
        jmp finished_judge_Dest
        

        err2Dest:
        mov dh,2
        jmp finished_judge_Dest
        
        finished_judge_Dest:
        

        cmp dh, 1
        jz error1
        cmp dh, 2
        jz error2

        ret

compare_num:
        push    ecx			
        push    ebx			
        push    eax			
    
        call slen
        mov ecx, eax
        mov eax, ebx
        call slen
        mov edx, eax
        pop eax
        push eax
        ; len_src -> ecx
        ; len_dest-> edx
        ; compare the two length of src and dest
        cmp ecx,edx
        je equalLen
        jg src_biger
        jl dest_biger
            

        equalLen:
        mov dh, [ebx]
        cmp dh, 0
        jz equalNum
        cmp [eax],dh
        jg src_biger
        jl dest_biger 
        inc eax
        inc ebx
        jmp equalLen

        equalNum:
        mov dl,2
        jmp finish_compare_Large
        
        src_biger:
        mov dl, 1
        jmp finish_compare_Large
        
        dest_biger:
        mov dl, 0
        jmp finish_compare_Large

        finish_compare_Large:
        pop     eax
        pop     ebx			
        pop     ecx			
        ret

print_res:
        call sprint
        mov eax, space
        call sprint
        mov  eax, ebx
        call sprint
        ret

div_func:
        		
        push    ecx			
        push    ebx			
        push    eax			
        
        mov dh,[count]

        loop1: 
        call AddZeroFunc
        mov dh,[count]
        inc dh
        mov [count],dh
        call compare_num
        cmp dl,0
        jz prepared
        jmp loop1
        ;循环add zero 至 larger than src
        ;一旦大于，就进入prepared，除以10，保证enough to minus
        
        prepared:
        call MoveZeroFunc
        mov dh,[count]
        dec dh
        mov [count],dh

        sub_div:
        call compare_num
        ; minus until it's not enough to minus
        ; then we remove a zero from the end of the number
        cmp dl,0
        jz countR
        call sub_string
        ; eax - ebx
        ; call the string minus function
        ; minus once, R add one
        mov ecx,eax
        mov eax,res_R
        ;below r++
        ;%%%%%%%%%%%%%%%%%%%%%%
        push    edx			
        push    ecx			
        push    ebx			
        push    eax			
        call    reverse
        mov     dl,[eax]
       
        add     dl, 1
        mov     [eax],dl
        mov     ecx,0

        add1_loop:
        ; judge whether exits carry
        cmp     byte [eax+ecx],3Ah
        ; 3Ah -> '9'+1
        ;判定输入的字符是否是0-9
        jl      finishe_add1
        mov     dl,[eax+ecx]
        sub     dl,10
        mov     [eax+ecx],dl
        inc     ecx 
        mov     dl,[eax+ecx]
        cmp     dl,0   
        jnz     continue_add1
        add     dl,'0'

        continue_add1:     
        add     dl,1
        mov     [eax+ecx],dl
        jmp     add1_loop

        finishe_add1:
        call    reverse
        pop     eax
        pop     ebx			
        pop     ecx			
        pop     edx			
        ;%%%%%%%%%%%%%%%%%%%%%%%


        mov eax,ecx
        jmp sub_div
        ;loop

        countR:
        ; not enough to minus then move zero
        mov dh,[count]
        cmp dh,0
        jz finish_NumDiv
        mov ecx,ebx
        mov ebx,res_R
        call AddZeroFunc
        ;R mul ten
        mov ebx,ecx
        ;prepared -> move zero
        jmp prepared

        finish_NumDiv:
        mov ebx,eax
        mov eax,res_R
        mov dh,[ebx]
        cmp dh,0
        jnz print_result
        mov dh,48
        mov [ebx],dh

        print_result:
        call    print_res
        pop     eax
        pop     ebx			
        pop     ecx			
        ret

AddZeroFunc:
        ;ebx mul ten
        ;-> add zero to the end of the number
        push    ecx			
        push    ebx			
        push    eax			
        mov eax,ebx
        call reverse
        mov ebx,eax
        call slen

        loop_Zero:
        ; move the char to the position to its next
        cmp eax,0
        jz add_Zero
        mov dl,[eax+ebx-1]
        mov [eax+ebx],dl
        dec eax
        jmp loop_Zero


        add_Zero:
        mov dl,'0'
        mov [ebx],dl
        mov eax,ebx
        call reverse
        ;reverse to get the right number
        mov ebx , eax
        pop     eax
        pop     ebx			
        pop     ecx			
        ret

reverse:
        ;reverse eax
        
        push    ecx			
        push    ebx			
        push    eax			
        mov ecx , 0 
        call slen
        mov ebx , eax

        pop eax
    
        put1: 
        cmp byte [eax+ecx],0
        jz finish_reverse1
        dec ebx
        mov dl,[eax+ebx]
        mov [temp+ecx],dl
        inc ecx
        jmp put1

        finish_reverse1:
        mov ecx,0;

        put2:
        cmp byte [eax+ecx], 0
        jz finish_reverse2
        mov dl,[temp+ecx]
        mov [eax+ecx],dl
        inc ecx
        jmp put2

        finish_reverse2:
        pop     ebx			
        pop     ecx			
       
        ret

MoveZeroFunc:
        ;move zero
        
        push    ecx			
        push    ebx			
        push    eax			

        mov eax,ebx
        call reverse
        mov ebx,eax
        call slen
        mov ecx,ebx

        loop_moveZero:
        cmp eax,0
        jz finish_move
        mov dl,[ebx+1]
        mov [ebx],dl
        ; move the chat to the position ahead of it
        dec eax
        inc ebx
        jmp loop_moveZero


        finish_move:
        mov ebx, ecx
        mov eax, ebx
        call reverse
        mov ebx, eax
        pop     eax
        pop     ebx			
        pop     ecx			
        
        ret
 
sub_string:
        ;string minus function
        ;eax minus ebx
        push    edx			
        push    ecx			
        push    ebx			
        push    eax	     
        
        call reverse
        mov ecx,eax
        mov eax,ebx
        call reverse
        mov ebx,eax
        mov eax,ecx

        sub_loop:
        ;loop until dl equals zero
        mov dl, [ebx]
        mov dh, [eax]
        cmp dl,0
        jz finish_sub
        sub dh,dl
        add dh,'0'
        mov [eax],dh
        cmp dh,'0'
        ; judge whether it's enough to minus
        jnl no_borrow
        mov ch,0

        borrow:
        ;borrow from higher 
        mov dh,[eax]
        ;borrow one from higher -> add ten
        add dh,10
        mov [eax],dh
        inc eax
        inc ch
        mov dh,[eax]
        sub dh,1
        mov [eax],dh
        cmp dh,'0'
        jnl borrow_2
        jmp borrow

        borrow_2:
        cmp ch,0
        jz no_borrow
        dec eax
        dec ch
        jmp borrow_2

        no_borrow:
        ;不借位的情况
        inc eax
        inc ebx
        jmp sub_loop
        

        finish_sub:
        pop     eax
        pop     ebx			
        call reverse
        mov ecx,eax
        mov eax,ebx
        call reverse
        mov ebx,eax
        mov eax,ecx
        pop     ecx			
        pop     edx			
        call remove_zero
        ret

remove_zero:
        ;clear the zero before the result
        push    edx			
        push    ecx			
        push    ebx			
        mov ecx, eax
        mov ebx,0
        mov dl,[eax]
        cmp dl,'0'
        
        jnz finish_noZero
        
        remove_:
        mov dl, [ecx]
        cmp dl, '0'
        jnz loop_remove
        inc ecx
        jmp remove_
        
        loop_remove:
        mov dl,[ecx]
        cmp dl, 0
        jz beforefinish
        mov [eax+ebx],dl
        inc ecx
        inc ebx
        jmp loop_remove
        
        beforefinish:
        mov dh,[eax+ebx]
        cmp dh,0
        jz finish_noZero
        mov dl,0
        mov [eax+ebx], dl
        inc ebx
        jmp beforefinish


        finish_noZero:
        pop     ebx			
        pop     ecx			
        pop     edx			
        ret