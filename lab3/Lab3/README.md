1. 清屏：
main.c:
在初始化之前把整个屏幕打印满空格，然后把指针移到最前面
显存指针 disp_指向第一个位置

2. TAB：
tty.c:
in_PROCESS方法中加入TAB判断
console.h:
定义TAB长度
console.c:
out_char进行修改
TAB输出，将cursor往后移动TAB-WIDTH
同时给TAB一个颜色区别于空格

此时还没实现删除时的问题

3. 删除：
书中代码的换行删除存在问题：\
光标无法回到正确位置\
所以使用一个栈来存每个走过的位置\
console.h:\
存储每一次操作的光标信息\
console.c:\
init_screen中初始化pos_stack的ptr指针\
console.h：\
新增两个方法,用于记录和获取指针的位置\
push_pos 和 pop_pos          
修改输出方法：\
case '\n'  '\b'  '\t'\
在cursor移动前push_pos, 在退格之前pop_pos来获取之前的位置

4. shift组合键：
keyboard.c中实现了

5. 清屏：
直接清屏，发现光标不会复位，所以要重新初始化screen
但是会发生报错，询问同学的回答是用户进程不能使用tty？
所以把testA 的进程从PROCS转为TASKS -- global.c


6. 查找：
global.c中添加mode：
0 -》 正常
1 -》 搜索
global.h也添加
extern int mode

- TestA也要改为mode == 0 清屏

- tty.c:
esc切换模式


- 搜索模式下我们要做的是两件事：
记录输入的字符
记录开始的位置

- console.h:
加入搜索输入栈，在CONSOLE与POSSTACK开辟变量，记录esc的位置

- 实时推出时光标复位：
console.c：
esc时判断模式，然后进行不同操作

- search：
暴力匹配

- 退出修改颜色：
console.c中case esc

- 回车屏蔽输入：
添加mode2
console .c中out_char()中判断模式

7. 撤销：
要记录每一步操作
global.c:
extern int control

- global.c:
PUBLIC int control

- keyboard.c - keyboard_read中添加
control = ctrl_l || ctrl_r

- console.c:
case 'z':
case 'Z':
doCtrlZ();

- console.h添加数据结构

- tty.c:
write时将out_char的ch压入操作记录栈

- console.c:
push_out_char同样操作

- 具体撤销使用redo操作