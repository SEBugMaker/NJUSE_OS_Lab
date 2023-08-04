;------------------------------------------
; void iprint(Integer number)
; 打印整数函数 (itoa)
iprint:
    push    eax             ; 将EAX的值保存在栈上以便函数运行后恢复
    push    ecx             ; 将ECX的值保存在栈上以便函数运行后恢复
    push    edx             ; 将EDX的值保存在栈上以便函数运行后恢复
    push    esi             ; 将ESI的值保存在栈上以便函数运行后恢复
    mov     ecx, 0          ; 最后要打印多少字节的计数器
 
divideLoop:
    inc     ecx             ; 所有要打印的字节计数 —— 字符数
    mov     edx, 0          ; 清空EDX
    mov     esi, 10         ; 将10放入ESI
    idiv    esi             ; EAX除以ESI
    add     edx, 48         ; 转化EDX的值为它的ASCII码表示 —— 调用除法指令后EDX保存余数
    push    edx             ; 将EDX (一个整数的字符串表示) 压到栈上
    cmp     eax, 0          ; 这个整数还能再被除么?
    jnz     divideLoop      ; 如果EAX不是0 跳转到divideLoop标签
 
printLoop:
    dec     ecx             ; 反向计数我们放在栈上的每个字节
    mov     eax, esp        ; 将栈指针放入EAX用于打印
    call    sprint          ; 调用字符串打印函数
    pop     eax             ; 移除栈上最后一个字符来移动ESP指向
    cmp     ecx, 0          ; 我们已经打印了栈上所有字节了么?
    jnz     printLoop       ; ECX不是0跳转到printLoop标签
 
    pop     esi             ; 用开始放到栈上的值恢复ESI
    pop     edx             ; 用开始放到栈上的值恢复EDX
    pop     ecx             ; 用开始放到栈上的值恢复ECX
    pop     eax             ; 用开始放到栈上的值恢复EAX
    ret
 
;------------------------------------------
; void iprintLF(Integer number)
; 带有换行符的整数打印函数 (itoa)
iprintLF:
    call    iprint          ; 调用数字打印函数
 
    push    eax             ; 当我们在这个函数使用EAX时将EAX压到栈上保留
    mov     eax, 0Ah        ; 将0AH放到EAX中 —— 0AH是ASCII码中的换行符
    push    eax             ; 将换行符压到栈上，这样我们就能得到地址
    mov     eax, esp        ; 将当前的栈指针传入EAX用于sprint
    call    sprint          ; 调用sprint函数
    pop     eax             ; 从栈上移除换行符
    pop     eax             ; 恢复调用函数前EAX的初始值
    ret
 
;------------------------------------------
; int slen(String message)
; 字符串长度计算函数
slen:
    push    ebx
    mov     ebx, eax
 
nextchar:
    cmp     byte [eax], 0
    jz      finished
    inc     eax
    jmp     nextchar
 
finished:
    sub     eax, ebx
    pop     ebx
    ret
 
;------------------------------------------
; void sprint(String message)
; 字符串打印函数
sprint:
    push    edx
    push    ecx
    push    ebx
    push    eax
    call    slen
 
    mov     edx, eax
    pop     eax
 
    mov     ecx, eax
    mov     ebx, 1
    mov     eax, 4
    int     80h
 
    pop     ebx
    pop     ecx
    pop     edx
    ret
 
;------------------------------------------
; void sprintLF(String message)
; 打印字符串和换行符函数
sprintLF:
    call    sprint
 
    push    eax         ; 当我们在这个函数中使用EAX时通过将EAX压入栈来进行保护
    mov     eax, 0AH    ; 将0AH移到EAX中 - 0AH是换行符的ascii码
    push    eax         ; 将换行符放到栈上，以便我们获取地址
    mov     eax, esp    ; 将当前栈指针的地址放到EAX寄存器中给sprint函数
    call    sprint      ; 调用sprint函数
    pop     eax         ; 从栈上移除换行符
    pop     eax         ; 恢复调用函数前EAX原本的值
    ret                 ; 返回程序
 
;------------------------------------------
; void exit()
; 退出程序并复原资源
quit:
    mov     ebx, 0
    mov     eax, 1
    int     80h
    ret