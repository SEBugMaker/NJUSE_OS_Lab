/**
 * console.c中out_char进行修改
 */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);
PRIVATE void pushCursor(CONSOLE* p_con,unsigned int pos);
PRIVATE unsigned int getAndPopCursor(CONSOLE* p_con);
PRIVATE void doSearch(CONSOLE *p_con);

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
    /**
     * init_screen中初始化pos_stack的ptr指针
     */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;
    //添加初始化内容
    p_tty->p_console->finalCursorBeforeEsc = p_tty->p_console->cursor;
    p_tty->p_console->cursor_record.idx = 0;
    p_tty->p_console->char_record.idx = 0;
    p_tty->p_console->char_record.esc_index = 0;
    for(int i = 0; i < SCREEN_SIZE; i++){
        p_tty->p_console->char_record.content[i] = ' ';
    }
    p_tty->p_console->cursor_record.len = SCREEN_SIZE;
	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2); //考虑多个控制台的情况

    if(mode == 2){
      //在搜索模式下按下enter键mode=2,
      // 此时屏蔽除esc以外所有输入
        if(ch == '\r'){
            mode = 0;
            p_con->char_record.idx = p_con->char_record.esc_index;
        } else{
            return;
        }
    }

	switch(ch) {
	case '\n':
        /**
         *  在cursor移动前push_pos, 在退格之前pop_pos来获取之前的位置
         */
	    if(mode == 0){
            //回车键直接把光标挪到了下一行的开头
            if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH) {
                //记录换行前的光标位置
                pushCursor(p_con,p_con->cursor);
                p_con->cursor = p_con->original_addr + SCREEN_WIDTH * ((p_con->cursor - p_con->original_addr) / SCREEN_WIDTH + 1);
            }
	    }
		else{ //查找模式
		    mode = 2;
            doSearch(p_con);
		}
		break;
	case '\b': //处理退格键
		if (p_con->cursor > p_con->original_addr && p_con->cursor_record.idx != 0) {
		    unsigned int idx = getAndPopCursor(p_con);
            if(mode==1){
                if(idx<p_con->finalCursorBeforeEsc){
                    pushCursor(p_con,idx);
                    break;
                }
            }
            //为了确保在查找模式下不会删到原文，
            // 一旦要删除的地方在原文范围内，就加回去
		    if(idx != (p_con->cursor_record.len + 1)){
              //如果不是错误值
		        int i = 0;
                while (p_con->cursor > idx){
                    p_con->cursor--;
                    *(p_vmem - 2 - 2 * i) = ' ';
                    *(p_vmem - 1 - 2 * i) = DEFAULT_CHAR_COLOR;
                    i++;
                }
		    }
		}
		break;
    case '\t':

        if(p_con->cursor < p_con->original_addr + p_con->v_mem_limit - TAB_WIDTH){//当前控制台的光标位置<当前控制台对应显存位置+当前控制台所占显存大小-tab长度
            pushCursor(p_con,p_con->cursor);
            for(int i = 0; i < TAB_WIDTH; i++){
                *p_vmem++ = ' ';
                *p_vmem++ = BLUE; //tab的颜色设置一个特殊的颜色，以便在search的时候和空格进行区分
                p_con->cursor++;
            }
        }
        break;
    case '\r': //用\r来作为ESC的代表字符
        if(mode == 1){ //进入查找模式
            p_con->char_record.esc_index = p_con->char_record.idx;
            p_con->finalCursorBeforeEsc = p_con->cursor;
        } else{ //退出查找模式
            if (p_con->cursor > p_con->original_addr){
                int i = 0;
                while(p_con->cursor > p_con->finalCursorBeforeEsc){
                    unsigned int idx = getAndPopCursor(p_con);
                    //注意在pop完之后在函数内部已经把index减1了
                    //相当于做了一次退格操作
                    while(p_con->cursor > idx){
                        p_con->cursor--;
                        *(p_vmem - 2 - 2 * i) = ' ';
                        *(p_vmem - 1 - 2 * i) = DEFAULT_CHAR_COLOR;
                        i++;
                    }
                }
            }
            //把匹配到的红色字符颜色改回去
            for(int i = 0; i < p_con->cursor; i++){
                if (*(u8*)(V_MEM_BASE + i * 2 + 1) == RED){
                    *(u8*)(V_MEM_BASE + i * 2 + 1) = DEFAULT_CHAR_COLOR;
                }
            }
        }
        break;
    case 'Z':
    case 'z':
        if(ctrl){
            doCtrlZ(p_con);
            return;
        }
	default:
		if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 1) {
            pushCursor(p_con, p_con->cursor); //先保存光标位置
            if(mode == 0 && p_con->cursor > p_con->finalCursorBeforeEsc){
                p_con->finalCursorBeforeEsc = p_con->cursor;
            }//只要不按esc，每一次都更新最后位置
			*p_vmem++ = ch;
			if(mode == 0 || ch == ' '){//为了能把tab和空格区分开
                *p_vmem++ = DEFAULT_CHAR_COLOR;
			} else{
                *p_vmem++ = RED;
			}
			p_con->cursor++;
		}
		break;
	}
    //超出屏幕范围将触发屏幕滚动
	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
    set_cursor(p_con->cursor);
    set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor 往控制台输出字符
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

/*======================================================================*
			   对记录光标位置的增加和删除操作
 *======================================================================*/
PRIVATE void pushCursor(CONSOLE* p_con,unsigned int pos){
    int curr = p_con->cursor_record.idx;
    if (curr < p_con->cursor_record.len){
        p_con->cursor_record.content[curr] = pos;
        p_con->cursor_record.idx++;
    }else{
        disp_str("Cursor Push Error");
    }
}
PRIVATE unsigned int getAndPopCursor(CONSOLE* p_con){
    int finalCursor = p_con->cursor_record.idx;
    if (finalCursor > 0){
        unsigned int res = p_con->cursor_record.content[finalCursor - 1];
        p_con->cursor_record.idx--;
        return res;
    }else{
        disp_str("Cursor Pop Error");
        return p_con->cursor_record.len + 1;//返回一个没有意义的错误值
    }
}
/*======================================================================*
			   查找模式进行字符串的匹配
 *======================================================================*/
PRIVATE void doSearch(CONSOLE *p_con){
    int len = p_con->cursor-p_con->finalCursorBeforeEsc; //待匹配字符串长度
    if(len == 0){
        return;
    }
    u8* curr_vmem;     //当前遍历到的字符
    u8* target_vmem;   //目标字符
    u8* curr_color;    //当前遍历到的字符颜色
    u8* target_color;  //目标字符颜色
    for(int i = 0; i < p_con->finalCursorBeforeEsc; i++){
        int found = 1;
        for(int j = 0; j < len; j++){
            curr_vmem = (u8*)(V_MEM_BASE + i * 2 + j * 2);
            curr_color = (u8*)(V_MEM_BASE + i * 2 + j * 2 + 1);
            target_vmem = (u8*)(V_MEM_BASE + p_con->finalCursorBeforeEsc * 2 + j * 2);
            target_color = (u8*)(V_MEM_BASE + p_con->finalCursorBeforeEsc * 2 + j * 2 + 1);
            if(*curr_vmem != *target_vmem || (*curr_vmem == ' ' && *curr_color != *target_color)){ //后一个或者是排除空格和tab
                found = 0;
                break;
            }
        }
        if(len > p_con->finalCursorBeforeEsc){//排除匹配字符比原文长的情况
            found = 0;
        }
        if(found == 1){
            for(int k = 0; k < len; k++){
                if(*(u8*)(V_MEM_BASE + i * 2 + k * 2) != ' ')
                    *(u8*)(V_MEM_BASE + i * 2 + k * 2 + 1) = RED;
            }
        }
    }
}
/*======================================================================*
			   撤销操作
撤销操作的思想就是先清屏，然后去掉要撤销的字符，把之前的字符都输出出来
 *======================================================================*/
PUBLIC void doCtrlZ(CONSOLE *p_con){
    if(mode == 0){
        clean_screen();
        p_con->cursor_record.idx = 0; //把指针记录的记录指针置为0
        p_con->cursor = disp_pos / 2; //disp_pos是console里实际的指针位置，cursor是记录它的
        //初始化指针
        flush(p_con);
        redo(p_con);
    }else if(mode == 1){
        p_con->cursor_record.idx = p_con->char_record.esc_index;
        disp_pos = p_con->finalCursorBeforeEsc * 2;
        for (int i = 0 ; i < SCREEN_SIZE; ++i){
            disp_str(" ");
        }
        disp_pos = p_con->finalCursorBeforeEsc * 2;
        p_con->cursor = disp_pos / 2; //输出的一个字节在显存里是两个字节，所以除2
        //初始化指针
        flush(p_con);
        redo(p_con);
    }
}

PUBLIC void redo(CONSOLE *p_con){
    int start;
    if (mode == 0){
        start = 0;
    }else if(mode == 1){
        start = p_con->char_record.esc_index;
    }
    p_con->char_record.idx -= 2; //这里的减2是减去z和前移一个位置，例如输入ab，再加上ctrl+z进入查找，content里存的是a，b，z，index=3
    if(p_con->char_record.idx <= 0){ //不能再撤销了
        p_con->char_record.idx = 0;
        return;
    }
    for(int i = start; i < p_con->char_record.idx; i++){
        out_char(p_con, p_con->char_record.content[i]);
    }
}
