
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "proc.h"
#include "global.h"
#include "proto.h"


PUBLIC	PROCESS	proc_table[NR_TASKS + NR_PROCS];

//由于用户任务是不能操作其他tty的，
// 所以需要将TaskA从任务变为进程
PUBLIC	TASK	task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "tty"},
	{TestA, STACK_SIZE_TESTA, "TestA"}};

PUBLIC  TASK    user_proc_table[NR_PROCS] = {

	{TestB, STACK_SIZE_TESTB, "TestB"},
	{TestC, STACK_SIZE_TESTC, "TestC"}};

PUBLIC	char		task_stack[STACK_SIZE_TOTAL];

PUBLIC	TTY		tty_table[NR_CONSOLES];
PUBLIC	CONSOLE		console_table[NR_CONSOLES];

PUBLIC	irq_handler	irq_table[NR_IRQ];

PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {sys_get_ticks};

PUBLIC int mode;
//用0表示正常模式，
// 1表示查找模式，
// 2表示仅接受ese（即在查找模式下按回车之后）

PUBLIC int ctrl; //用0表示正常，1表示按下ctrl
