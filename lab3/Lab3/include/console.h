
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_
#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */
#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80
/**
 * console.h中定义TAB长度
 */
#define TAB_WIDTH 4

#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */

/**
 * 存储每一次操作的光标信息
 */
typedef struct cursor_record{
    unsigned int idx;//当前的下标
    unsigned int len;//数组的长度
    unsigned int content[SCREEN_SIZE];//存储每一次的光标信息
}CURSOR_RECORD;

typedef struct char_record{
    int idx;//当前的下标
    int esc_index;
    //记录/r的下标
    // esc要单独记录，因为其不参与撤回
    char content[SCREEN_SIZE];//储存每一步的字符
}CHAR_RECORD;

/* CONSOLE */
typedef struct s_console
{
    /**
     *
     */
    unsigned int current_start_addr;	/* 当前显示到了什么位置	  */
    unsigned int original_addr;		/* 当前控制台对应显存位置 */
    unsigned int v_mem_limit;		/* 当前控制台占的显存大小 */
    unsigned int cursor;			    /* 当前光标位置 */
    CURSOR_RECORD cursor_record;                 //存储每一次操作的光标信息，以便退格
    CHAR_RECORD char_record;                //存储每一次操作输入的字符，用来撤回
    unsigned int finalCursorBeforeEsc;     //进入esc前最后一个光标信息，以便退出esc后能返回
}CONSOLE;

#endif /* _ORANGES_CONSOLE_H_ */
