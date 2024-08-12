#ifndef LOADER_H
#define LOADER_H

#include "comm/types.h"
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"

void protect_mode_entry(void);

// 内存检测信息结构,实际上是一个中间变量，最后的结果存储在_boot_info_t
typedef struct SMAP_entry {
    uint32_t BaseL; // base address uint64_t
    uint32_t BaseH;
    uint32_t LengthL; // length uint64_t
    uint32_t LengthH;
    uint32_t Type; // entry Type
    uint32_t ACPI; // extended
}__attribute__((packed)) SMAP_entry_t;
//__attribute__((packed))`告诉编译器按照紧凑方式对齐结构体的成员，避免结构体成员之间插入填充以保证结构体大小最小

extern boot_info_t boot_info;   //全局声明

#endif // LOADER_H