/**
 * 自己动手写操作系统
 *
 * 系统引导部分,启动时由硬件加载运行,然后完成对二级引导程序loader的加载
 * 该部分程序存储于磁盘的第1个扇区,在计算机启动时将会由BIOS加载到0x7c00处
 * 之后,将由BIOS跳转至0x7c00处开始运行
 */
	#include "boot.h"

  	// 16位代码,务必加上
  	.code16
 	.text
	.global _start       //这个标签指明了代码的入口为_start
	.extern boot_entry
_start:
	mov $0, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %fs
	mov %ax, %gs

	//使用0x7c00之前的空间作为栈,大约有30KB的RAM,足够boot和loader使用
	mov $_start, %esp

	//加载loader,只支持磁盘一
	//用寄存器设置读取的信息,详情见手册
read_loader:
	//将磁盘中的loader代码读取到内存中0x8000的位置,ES:BX形式由于前面es设为0,只需要给出bx即可
	mov $0x8000, %bx
	mov $0x2, %ah    //固定值
	//接下来给出磁盘中的柱面和扇区cx的高八位ch为柱面,低八位cl为扇区
	mov $0x2, %cx       //第0个柱面,第二个扇区
	//接下来al为读取的扇区数,设置大一些,防止loader代码没有全部加载进去
	mov $64, %al    //32kb
	//接下来在dx高八位dh为磁头(此处为0),低八位dl为驱动器(00H~7FH为软盘；80H~0FFH为硬盘)
	mov $0x0080, %dx
	//使用int 13中断告诉BIOS要读磁盘了
	int $0x13
	//最后使用jc命令判断,int 13出口为0则成功，为1则失败，jc命令会判断如果为1跳转到后面的标签中重复执行
	jc read_loader

	//跳转到c代码中再进行一些处理
	jmp boot_entry
	
	jmp .
	
	// 引导结束段
	.section boot_end, "ax"
boot_sig: .byte 0x55, 0xaa



//问题一：必须定期点右下角的build，不然有调试的问题
   