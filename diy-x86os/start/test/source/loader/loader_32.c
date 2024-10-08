//接来下要读磁盘，因为bios中的代码是16位，现在已经进入保护模式，所以bios的代码已经无法使用

#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "loader.h"
#include"comm/elf.h"

//这个函数等价于read_loader 
static void read_disk(int sector, int sector_count, uint8_t * buf) {
    outb(0x1F6, (uint8_t) (0xE0));

	outb(0x1F2, (uint8_t) (sector_count >> 8));
    outb(0x1F3, (uint8_t) (sector >> 24));		// LBA参数的24~31位
    outb(0x1F4, (uint8_t) (0));					// LBA参数的32~39位
    outb(0x1F5, (uint8_t) (0));					// LBA参数的40~47位

    outb(0x1F2, (uint8_t) (sector_count));
	outb(0x1F3, (uint8_t) (sector));			// LBA参数的0~7位
	outb(0x1F4, (uint8_t) (sector >> 8));		// LBA参数的8~15位
	outb(0x1F5, (uint8_t) (sector >> 16));		// LBA参数的16~23位

	outb(0x1F7, (uint8_t) 0x24);

	// 读取数据
	uint16_t *data_buf = (uint16_t*) buf;
	while (sector_count-- > 0) {
		// 每次扇区读之前都要检查，等待数据就绪
		while ((inb(0x1F7) & 0x88) != 0x8) {}

		// 读取并将数据写入到缓存中
		for (int i = 0; i < SECTOR_SIZE / 2; i++) {
			*data_buf++ = inw(0x1F0);
		}
	}
}

/**
 * 死机
 */
static void die (int code) {
    for (;;) {
    }
}

//这里之所以kernel需要加载.elf而前面的boot和loader不需要
//是因为前两个过于简单，所以cmake将其直接生成为.bin,而kernel被上称为.elf
static uint32_t reload_elf_file (uint8_t * file_buffer) {
    // 读取的只是ELF文件，不像BIN那样可直接运行，需要从中加载出有效数据和代码
    // 简单判断是否是合法的ELF文件
    Elf32_Ehdr * elf_hdr = (Elf32_Ehdr *)file_buffer;
    if ((elf_hdr->e_ident[0] != ELF_MAGIC) || (elf_hdr->e_ident[1] != 'E')
        || (elf_hdr->e_ident[2] != 'L') || (elf_hdr->e_ident[3] != 'F')) {
        return 0;
    }

    // 然后从中加载程序头，将内容拷贝到相应的位置
    for (int i = 0; i < elf_hdr->e_phnum; i++) {
        Elf32_Phdr * phdr = (Elf32_Phdr *)(file_buffer + elf_hdr->e_phoff) + i;
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

		// 全部使用物理地址，此时分页机制还未打开
        uint8_t * src = file_buffer + phdr->p_offset;
        uint8_t * dest = (uint8_t *)phdr->p_paddr;
        for (int j = 0; j < phdr->p_filesz; j++) {
            *dest++ = *src++;
        }

		// memsz和filesz不同时，后续要填0
		dest= (uint8_t *)phdr->p_paddr + phdr->p_filesz;
		for (int j = 0; j < phdr->p_memsz - phdr->p_filesz; j++) {
			*dest++ = 0;
		}
    }

    return elf_hdr->e_entry;   //0x10000处
}

#define PDE_P     (1 << 0)    //有效
#define PDE_W     (1 << 1)    //可写
#define PDE_PS    (1 << 7)    //4MB

#define CR4_PSE   (1 << 4)    //巨页
#define CR0_PG    (1 << 31)   //开启分页的标志


//打开分页机制
void enable_page_mode(void){
    //定义巨页的一级表项,4KB对齐
    //第0个表项的最高10位表示映射的起始地址，这里不写表示从0开始,即0-4M不做映射，逻辑地址等价于物理地址
    static uint32_t page_dir[1024] __attribute__((aligned(4096)))={
        [0]=PDE_P | PDE_W | PDE_PS | 0           //只是简单的对0-4M进行映射(同地址映射)
    };
    //设置cr4寄存器,设置其中PS位为1，打开巨页模式
    uint32_t cr4=read_cr4();
    write_cr4(cr4 | CR4_PSE);

    //设置cr3寄存器，将page_dir加载进去
    write_cr3((uint32_t)page_dir);

    //开启分页机制
    write_cr0(read_cr0() | CR0_PG);
}

void load_kernel(void){ 
    //将内核代码加载出来,读到1M的位置,此时这个位置上的数据是.elf类型
    read_disk(100,500,(uint8_t*)SYS_KERNEL_LOAD_ADDR);
    uint32_t kernel_entry=reload_elf_file((uint8_t *)SYS_KERNEL_LOAD_ADDR);
	if(kernel_entry==0){
		die(-1);
	}

    //打开分页机制
    enable_page_mode();
	
	((void(*)(boot_info_t*))kernel_entry)(&boot_info);
}  