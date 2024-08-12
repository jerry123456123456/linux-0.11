//在这里需要完成实模式到保护模式的切换，所以先试用_16
//16位的引导代码

//16位代码。必须加上放在开头，防止生成32位指令
__asm__(".code16gcc");
#include"loader.h"
//贱种stdint...老是出来

boot_info_t boot_info;   //要作为参数传递到kernel里面

static void show_msg(const char* msg) {
    char c;

    //使用bios写显存，持续往下写
    while ((c = *msg++) != '\0') {
        __asm__ __volatile__(
            "mov $0xe, %%ah\n\t"     //%%是转义字符
            "mov %[ch], %%al\n\t"    //[ch]是占位符，在下面的,输入操作数
            "int $0x10"::[ch]"r"(c)  //::是输入输出约束分隔符，后面制定了输入输出如何约束，r表示将c放在通用寄存器中(任意寄存器)
            //`::`：没有输出操作数用::
        );
    }
}

///###非常重要！！！！！
/*
这个内存检测函数是检测所有内存，我们在image脚本中一共为qemu配置了128M内存
其中函数返回的结果是所有的可用内存，0x7c00以及0x8000的代码包含在其中
而在654336到1048576是用作显存，bios代码取得映射之类的
所以，剩下一共两块可用的区域
*/
//开始内存检测
static void detect_memory(void) {
    uint32_t contId = 0;     // `contID`：`contID`是用来记录内存检测的控制ID的变量。在这段代码中，`contID`用于控制内存检测过程中的迭代，通过不断改变`contID`的值来获取不同的内存信息。当`contID`为0时，表示内存检测结束。
    SMAP_entry_t smap_entry;  //用于检测的中间信息
    int signature, bytes;
    //`signature`：`signature`用于存储从中断0x15返回的标识符。在这段代码中，`signature`的值用来判断中断返回的信息是否有效。如果`signature`的值不等于`0x534D4150`，则表示返回的信息无效，可能是由于中断调用失败或返回的数据不符合预期
    //`bytes`：`bytes`用于存储从中断0x15返回的字节数。在这段代码中，`bytes`的值用来判断返回的内存信息的长度是否符合预期。如果`bytes`的值大于20且`entry`的`ACPI`字段的最低位为0，则表示返回的信息有效且符合条件，可以继续处理

    show_msg("try to detect memory:");

    boot_info.ram_region_count = 0;
    //循环依次将检测到的内存块信息存储到_boot_info_t中
    for (int i = 0; i < BOOT_RAM_REGION_MAX; i++) {
        //通过定义一个指向结构体的指针，可以在不直接操作结构体变量的情况下，通过指针间接地访问和修改结构体的成员。这种方式在处理结构体数据时更加灵活和方便，可以避免不必要的数据复制和提高代码的可读性。
        SMAP_entry_t* entry = &smap_entry;

        __asm__ __volatile__("int $0x15"  //0x15中断可以获取内存状态
            //`:`：单冒号`:`表示有输入或输出操作数，即内联汇编代码中有显式的输入或输出约束。在这种情况下，后面会列出具体的输入和输出约束，告诉编译器如何将变量和寄存器关联起来
                //前缀`=`表示这是一个输出操作数，而没有前缀`=`的操作数表示输入操作数
            : "=a"(signature), "=c"(bytes), "=b"(contId)
            : "a"(0xE820), "b"(contId), "c"(24), "d"(0x534D4150), "D"(entry));
        //总的来说这段代码是传入5个变量到寄存器中在调用0x15中断，返回的结束输出到3个变量中

        if (signature != 0x534D4150) {
            show_msg("failed.\r\n");
            return;
        }

        // todo: 20字节
        if (bytes > 20 && (entry->ACPI & 0x0001) == 0) {
            continue;
        }

        //保存RAM信息，只取32位，空间有限无需考虑更大容量的情况
        if (entry->Type == 1) {   //查到了
            boot_info.ram_region_cfg[boot_info.ram_region_count].start = entry->BaseL;  //只取低32位
            boot_info.ram_region_cfg[boot_info.ram_region_count].size = entry->LengthL; //只取低32位
            boot_info.ram_region_count++;
        }

        if (contId == 0) {
            break;
        }
    }
    show_msg("ok.\r\n");
}

// GDT表。临时用，后面内容会替换成自己的
uint16_t gdt_table[][4] = {
    {0, 0, 0, 0},
    {0xFFFF, 0x0000, 0x9A00, 0x00CF},
    {0xFFFF, 0x0000, 0x9200, 0x00CF},
};


//进入保护模式的函数
static void enter_protect_mode() {
    //第一步关中断
    cli();

    //第二步要开启A20地址线，是的可以访问1M以上的空间
    //做法：读取0x92端口的值，与0x2与运算，再写回去
    uint8_t v = inb(0x92);
    outb(0x92, v | 0x2);

    //第三步，加载GDT表
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));

    //第四步，把cr0寄存器的最低位置1，进入保护模式
    uint32_t cr0 = read_cr0();
    write_cr0(cr0 | (1 << 0));
    //cr0 = read_cr0();

    //由于原来实模式下cpu指令周期性分段执行（取址，执行等），切到保护模式要清空指令，进行远跳转
    // 长跳转进入到保护模式
    // 使用长跳转，以便清空流水线，将里面的16位代码给清空
    far_jump(8, (uint32_t)protect_mode_entry);
}

void loader_entry(void) {
    show_msg("....loading....\r\n");
    detect_memory();
    enter_protect_mode();
}