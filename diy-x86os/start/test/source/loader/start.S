/**
 * 自己动手写操作系统
 *
 * 二级引导，负责进行硬件检测，进入保护模式，然后加载内核，并跳转至内核运行
 */
  	// 16位代码，务必加上
  	.code16
 	.text
 	.extern loader_entry
	.global _start   //这个标签指明了代码的入口为_start
_start:
    //栈和段等沿用之前的设置
    //不能直接跳到loader_entry因为需要预先知道在哪
    //boot没有这个能力，所以直接用汇编，以便使_start处于整个bin文件开头，这样boot直接跳到开头就可以了
    jmp loader_entry

	//接下来进入保护模式，使用32位代码
	.code32
	.text
	//保护模式的入口处
	.global protect_mode_entry
	.extern load_kernel
protect_mode_entry:
	//进入32位模式下运行
	mov $16, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs
	//跳到c语言中去执行
	jmp $8, $load_kernel
