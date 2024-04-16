
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule() {
	PROCESS *p;
	int greatest_ticks = 0;
	
	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++) {
			
			if (p->wakeup > 0 || p->block == 1) continue;
			// 正在睡眠/阻塞的进程不会被执行（也就是不会被分配时间片）
			if (p->ticks > greatest_ticks) {
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}
		// 如果都是 0，那么需要重设 ticks
		if (!greatest_ticks) {
			for (p = proc_table; p < proc_table + NR_TASKS + NR_PROCS; p++) {
				if (p->ticks > 0) continue; // >0 还进入这里只能说明它被阻塞了
				p->ticks = p->priority;
			}
		}
	}
}
	
/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC void sys_print(char * s, int len){
	CONSOLE *p_con = console_table;
	for (int i = 0; i < len; i++) {
		out_char(p_con, s[i]);
	}
}

// 如果我没有理解错的话,p_proc_ready 是已经在处理机器上运行的进程
PUBLIC void sys_sleep(int milli){
int ticks = milli / 1000 * HZ * 10;
	p_proc_ready->wakeup = ticks;
	schedule();
} 

PUBLIC void sys_p(void * mutex){
	disable_int();
	Semaphore* semaphore_mutex = (Semaphore *) mutex;
	semaphore_mutex->value--;
	if(semaphore_mutex->value < 0){
		sleep_process(semaphore_mutex);
	}
	enable_int();
}

PUBLIC void sys_v(void* mutex){
	disable_int();
	Semaphore * semaphore_mutex = (Semaphore *) mutex;
	semaphore_mutex->value++;
	if(semaphore_mutex->value <= 0){
		wake_process(mutex);
	} 
	enable_int();
}

PUBLIC int is_runable(PROCESS * p){
	if(p->wakeup <= get_ticks() && p->block == 0 && p->done == 0){
		return 1;
	}
	else{
		return 0;
	}
}


// 选择数组而不是链表
PUBLIC void sleep_process(Semaphore* mutex){
	mutex->queue[-(mutex->value) - 1] = p_proc_ready;
	p_proc_ready->block = 1;
	// next one
	schedule();
}
// 唤醒这里可以选择是按顺序唤醒还是有个优先级
PUBLIC void wake_process(void * mutex){
	Semaphore* semaphore_wake = (Semaphore *)mutex;
	PROCESS* wake = semaphore_wake->queue[0];
	wake->block = 0;
	for(int i = -(semaphore_wake->value); i > 0; i--){
		semaphore_wake->queue[i - 1] = semaphore_wake->queue[i];
	}
}
// 注意有个进程是A进程，不参与read and write， 这个可以在main里面设计，这里我们故且认为那是最后一个
PUBLIC void check_is_all_done(){
	PROCESS * now;
	int flag = 1;
	for(now = proc_table; now < proc_table + NR_TASKS - 1; now++){
		if(now->done == 0){
			return;
		}
	}
	for(now = proc_table; now < proc_table + NR_TASKS - 1; now++){
		now->done = 0;
	}
	disp_str("res_start");
}
